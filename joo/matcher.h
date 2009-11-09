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

#ifndef INCLUDE_JOO_MATCHER_H
#define INCLUDE_JOO_MATCHER_H

#include <unistd.h>  // ssize_t

#include "macros.h"
#include "term.h"

typedef struct joo_pushed_data {
  JooList  entry;

  char    *data;
  int      size;
} JooPushedData;

typedef int (*JooNoMatchFunc) (void *user_data);
typedef int (*JooMatchFunc) (void *user_data, JooPushedData *pushed_data);

typedef struct joo_matcher_entry JooMatcherEntry;

typedef struct joo_matcher {
  JooMatcherEntry *matchers;

  JooNoMatchFunc   no_match_func;
  void            *no_match_user_data;
} JooMatcher;

JOO_BEGIN_EXTERN

JooMatcher *joo_matcher_new   (JooNoMatchFunc no_match_func,
                               void *no_match_user_data);
void        joo_matcher_free  (JooMatcher *matcher);

int         joo_matcher_add   (JooMatcher *matcher, JooMatchFunc func,
                               void *user_data, JooTerm *terms);

int         joo_matcher_match (JooMatcher *matcher, const char *buf,
                               ssize_t bufsize);

JOO_END_EXTERN

#endif //INCLUDE_JOO_MATCHER_H

// vim:set et sw=2 sts=2:
