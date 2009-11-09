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

#ifndef INCLUDE_JOO_TERM_H
#define INCLUDE_JOO_TERM_H

#include <stdio.h>   // NULL
#include <string.h>  // memset
#include <unistd.h>  // ssize_t

#include "list.h"
#include "macros.h"

typedef enum {
  JOO_END,
  JOO_ATOM,
  JOO_STRING,
  JOO_BINARY,
  JOO_TUPLE_START,
  JOO_TUPLE_END
} JooType;

typedef struct joo_term {
  JooList entry;

  JooType type;
  int     push;

  union {
    char *text;
    struct {
      char    *data;
      ssize_t  size;
    } binary;
  } value;

  ssize_t num_children;
} JooTerm;

JOO_BEGIN_EXTERN

static inline JooTerm
joo_term_build (JooType  type,
                int      push,
                char    *data,
                ssize_t  binary_size)
{
  JooTerm term;
  memset (&term, 0, sizeof (JooTerm));
  term.type = type;
  term.push = push;
  if ((type == JOO_ATOM) || (type == JOO_STRING)) {
    term.value.text = data;
  } else if (type == JOO_BINARY) {
    term.value.binary.data = data;
    term.value.binary.size = binary_size;
  }
  return term;
}

#define joo_end() (joo_term_build (JOO_END, 0, NULL, 0))

#define joo_atom(text)  (joo_term_build (JOO_ATOM, 0, (text), 0))
#define joo_push_atom() (joo_term_build (JOO_ATOM, 1, NULL,   0))

#define joo_string(text)  (joo_term_build (JOO_STRING, 0, (text), 0))
#define joo_push_string() (joo_term_build (JOO_STRING, 1, NULL,   0))

#define joo_binary(bin, size) (joo_term_build (JOO_BINARY, 0, (bin), (size)))
#define joo_push_binary()     (joo_term_build (JOO_BINARY, 1, NULL,  0))

#define joo_tuple_start() (joo_term_build (JOO_TUPLE_START, 0, NULL, 0))
#define joo_tuple_end()   (joo_term_build (JOO_TUPLE_END,   0, NULL, 0))

int      joo_term_count_children (JooTerm *terms);
JooTerm *joo_term_dup            (const JooTerm *term);
void     joo_term_free           (JooTerm *term);

JOO_END_EXTERN

#endif //INCLUDE_JOO_TERM_H

// vim:set et sw=2 sts=2:
