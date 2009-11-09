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
#include <errno.h>
#include <stdio.h>   // NULL
#include <string.h>  // memcpy

#include "list.h"
#include "term.h"

#define STACK_SIZE 128

int
joo_term_count_children (JooTerm *terms)
{
  JooTerm  *stack[STACK_SIZE];
  JooTerm **parent = stack;

  *parent = NULL;

  for (JooTerm *term = terms; term->type != JOO_END; term++) {
    if (term->type == JOO_TUPLE_END) {
      if ((! *parent) || ((*parent)->type != JOO_TUPLE_START)) {
        errno = EINVAL;
        return -1;
      }

      --parent;

      continue;
    }

    if (*parent)
      (*parent)->num_children++;

    if (term->type == JOO_TUPLE_START) {
      if (++parent >= stack+STACK_SIZE) {
        errno = EINVAL;
        return -1;
      }

      *parent = term;
      (*parent)->num_children = 0;
    }
  }

  if (parent != stack) {
    errno = EINVAL;
    return -1;
  }

  return 0;
}

JooTerm *
joo_term_dup (const JooTerm *term)
{
  int errsv;

  JooTerm *dup;

  assert (term != NULL);

  dup = malloc (sizeof (JooTerm));
  if (! dup)
    return NULL;

  joo_list_init (&dup->entry);

  dup->type = term->type;
  dup->push = term->push;

  if ((term->type == JOO_ATOM) || (term->type == JOO_STRING)) {
    if (term->value.text) {
      if (! (dup->value.text = strdup (term->value.text)))
        goto alloc_error;
    } else {
      dup->value.text = NULL;
    }
  } else if (term->type == JOO_BINARY) {
    if (term->value.binary.data) {
      if (! (dup->value.binary.data = malloc (term->value.binary.size)))
        goto alloc_error;
      memcpy (dup->value.binary.data, term->value.binary.data,
              term->value.binary.size);
      dup->value.binary.size = term->value.binary.size;
    } else {
      dup->value.binary.data = NULL;
    }
  }

  dup->num_children = term->num_children;

  return dup;

alloc_error:
  errsv = errno;
  free (dup);
  errno = errsv;
  return NULL;
}

void
joo_term_free (JooTerm *term)
{
  assert (term != NULL);

  if ((term->type == JOO_ATOM) || (term->type == JOO_STRING)) {
    free (term->value.text);
  } else if (term->type == JOO_BINARY) {
    free (term->value.binary.data);
  }

  joo_list_cut (&term->entry);

  free (term);
}

