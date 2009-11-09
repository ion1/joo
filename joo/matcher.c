/* joo – a wrapper for ei
 *
 * Copyright © 2009 Johan Kiviniemi
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <assert.h>
#include <ei.h>
#include <stdio.h>   // NULL
#include <stdlib.h>  // malloc etc.
#include <string.h>  // memset
#include <unistd.h>  // ssize_t

#include "list.h"
#include "matcher.h"
#include "term.h"

struct joo_matcher_entry {
  JooList       entry;

  JooMatchFunc  func;
  void         *user_data;
  JooTerm      *terms;
};

static JooMatcherEntry *
joo_matcher_entry_new (JooMatchFunc  func,
                       void         *user_data)
{
  int errsv;

  JooMatcherEntry *entry = malloc (sizeof (JooMatcherEntry));
  if (! entry)
    return NULL;

  joo_list_init (&entry->entry);

  entry->func = func;
  entry->user_data = user_data;

  entry->terms = (JooTerm *)joo_list_new ();
  if (! entry->terms)
    goto list_alloc_failed;

  return entry;

list_alloc_failed:
  errsv = errno;
  free (entry);
  errno = errsv;
  return NULL;
}

static void
joo_matcher_entry_free (JooMatcherEntry *entry)
{
  JOO_LIST_FOREACH_SAFE (entry->terms, term) {
    joo_term_free (term);
  }
  free (entry->terms);

  joo_list_cut (&entry->entry);

  free (entry);
}

static int
dummy_no_match_func (void *no_match_user_data)
{
  (void)no_match_user_data;
  return -1;
}

JooMatcher *
joo_matcher_new (JooNoMatchFunc  no_match_func,
                 void           *no_match_user_data)
{
  int errsv;

  JooMatcher *matcher = malloc (sizeof (JooMatcher));
  if (! matcher)
    return NULL;

  matcher->matchers = (JooMatcherEntry *)joo_list_new ();
  if (! matcher->matchers)
    goto list_alloc_failed;

  matcher->no_match_func = no_match_func ? no_match_func : dummy_no_match_func;
  matcher->no_match_user_data = no_match_user_data;

  return matcher;

list_alloc_failed:
  errsv = errno;
  free (matcher);
  errno = errsv;
  return NULL;
}

void
joo_matcher_free (JooMatcher *matcher)
{
  assert (matcher != NULL);

  JOO_LIST_FOREACH_SAFE (matcher->matchers, entry) {
    joo_matcher_entry_free (entry);
  }
  free (matcher->matchers);

  free (matcher);
}

int
joo_matcher_add (JooMatcher   *matcher,
                 JooMatchFunc  func,
                 void         *user_data,
                 JooTerm      *terms)
{
  int errsv;
  if (joo_term_count_children (terms) < 0)
    return -1;

  JooMatcherEntry *entry = joo_matcher_entry_new (func, user_data);
  if (! entry)
    return -1;

  for (JooTerm *term = terms; term->type != JOO_END; term++) {
    if (term->type == JOO_TUPLE_END)
      continue;

    JooTerm *dup = joo_term_dup (term);
    if (! dup)
      goto term_alloc_failed;

    joo_list_add_before (&entry->terms->entry, &dup->entry);
  }

  joo_list_add_before (&matcher->matchers->entry, &entry->entry);

  return 0;

term_alloc_failed:
  errsv = errno;
  joo_matcher_entry_free (entry);
  errno = errsv;
  return -1;
}

static JooPushedData*
push_new (char *data, int size)
{
  JooPushedData *entry = malloc (sizeof (JooPushedData));
  if (! entry)
    return NULL;

  joo_list_init (&entry->entry);

  entry->data = data;
  entry->size = size;

  return entry;
}

static void
push_flush (JooPushedData *push)
{
  JOO_LIST_FOREACH_SAFE (push, entry) {
    free (entry->data);

    joo_list_cut (&entry->entry);
    free (entry);
  }
}

static void
push_cleanup (JooPushedData **push)
{
  if (! *push)
    return;

  push_flush (*push);
  free (*push);
}

typedef int (*EiDecodeTextFunc) (const char *buf, int *index, char *str);

static int
match_text (EiDecodeTextFunc  func,
            const char       *buf,
            int              *indexp,
            JooTerm          *term,
            JooPushedData    *push)
{
  int   type;
  int   size;
  char *text;

  if (! (term->value.text || term->push))
    // No comparison or push, just verify type.
    return func (buf, indexp, NULL);

  if (ei_get_type (buf, indexp, &type, &size) < 0)
    return -1;

  size++; // Null-terminated

  if (! (text = malloc (size)))
    return -1;

  if (func (buf, indexp, text) < 0) {
    free (text);
    return -1;
  }

  if (term->value.text) {
    if (strcmp (term->value.text, text)) {
      free (text);
      return -1;
    }
  }

  if (term->push) {
    JooPushedData *entry = push_new (text, size);
    if (! entry) {
      free (text);
      return -1;
    }

    joo_list_add_before (&push->entry, &entry->entry);
    return 0;

  } else {
    free (text);
    return 0;
  }
}

static int
match_binary (const char    *buf,
              int           *indexp,
              JooTerm       *term,
              JooPushedData *push)
{
  int   type;
  int   size;
  char *data;

  if (! (term->value.binary.data || term->push))
    // No comparison or push, just verify type.
    return ei_decode_binary (buf, indexp, NULL, NULL);

  if (ei_get_type (buf, indexp, &type, &size) < 0)
    return -1;

  if (! (data = malloc (size)))
    return -1;

  if (ei_decode_binary (buf, indexp, data, NULL) < 0) {
    free (data);
    return -1;
  }

  if (term->value.binary.data) {
    if ((term->value.binary.size != size) ||
        (memcmp (term->value.binary.data, data, size))) {
      free (data);
      return -1;
    }
  }

  if (term->push) {
    JooPushedData *entry = push_new (data, size);
    if (! entry) {
      free (data);
      return -1;
    }

    joo_list_add_before (&push->entry, &entry->entry);
    return 0;

  } else {
    free (data);
    return 0;
  }
}

static int
match_tuple (const char    *buf,
             int           *indexp,
             JooTerm       *term)
{
  int arity;

  if (term->push)
    // No push supported.
    return -1;

  if (ei_decode_tuple_header (buf, indexp, &arity) < 0)
    return -1;

  if (arity != term->num_children)
    return -1;

  return 0;
}

int
joo_matcher_match (JooMatcher *matcher,
                   const char *buf,
                   ssize_t     bufsize)
{
  int           index = 0;
  int           index_restart;

  __attribute__ ((cleanup(push_cleanup)))
    JooPushedData *push = (JooPushedData *)joo_list_new ();
  if (! push)
    return matcher->no_match_func (matcher->no_match_user_data);

  if (ei_decode_version (buf, &index, NULL) < 0)
    return matcher->no_match_func (matcher->no_match_user_data);
  index_restart = index;

  JOO_LIST_FOREACH (matcher->matchers, entry) {
    int found = 1;

    index = index_restart;

    JOO_LIST_FOREACH (entry->terms, term) {
      if (term->type == JOO_ATOM) {
        if (match_text (ei_decode_atom, buf, &index, term, push) < 0) {
          found = 0;
          break;
        }
      } else if (term->type == JOO_STRING) {
        if (match_text (ei_decode_string, buf, &index, term, push) < 0) {
          found = 0;
          break;
        }
      } else if (term->type == JOO_BINARY) {
        if (match_binary (buf, &index, term, push) < 0) {
          found = 0;
          break;
        }
      } else if (term->type == JOO_TUPLE_START) {
        if (match_tuple (buf, &index, term) < 0) {
          found = 0;
          break;
        }
      } else {
        found = 0;
        break;
      }

      assert (index <= bufsize);
    }

    if (found)
      return entry->func (entry->user_data, push);
  }

  return matcher->no_match_func (matcher->no_match_user_data);
}

// vim:set et sw=2 sts=2:
