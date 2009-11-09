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

#ifndef INCLUDE_JOO_LIST_H
#define INCLUDE_JOO_LIST_H

#include <malloc.h>
#include <stdio.h>   // NULL

#include "macros.h"

#define JOO_LIST_FOREACH(list, iter) \
  for (typeof (list) iter = (typeof (list))(list)->entry.next; \
       iter != (list); iter = (typeof (list))iter->entry.next)

#define JOO_LIST_FOREACH_SAFE(list, iter) \
  for (typeof (list) iter = (typeof (list))(list)->entry.next, \
                     iter##_next_ = (typeof (list))iter->entry.next; \
       iter != (list); \
       iter = iter##_next_, iter##_next_ = (typeof (list))iter->entry.next)

typedef struct joo_list JooList;

struct joo_list {
  JooList *prev;
  JooList *next;
};

JOO_BEGIN_EXTERN

static inline void
joo_list_init (JooList *list)
{
  list->prev = list;
  list->next = list;
}

static inline JooList *
joo_list_new (void)
{
  JooList *list = malloc (sizeof (JooList));
  if (! list)
    return NULL;

  joo_list_init (list);

  return list;
}

static inline JooList *
joo_list_cut (JooList *list)
{
  if ((list->prev == list) && (list->next == list))
    return NULL;

  list->prev->next = list->next;
  list->next->prev = list->prev;

  list->prev = list;
  list->next = list;

  return list;
}

static inline void
joo_list_add_before (JooList *pos, JooList *list)
{
  joo_list_cut (list);

  list->prev = pos->prev;
  pos->prev->next = list;
  pos->prev = list;
  list->next = pos;
}

static inline void
joo_list_add_after (JooList *pos, JooList *list)
{
  joo_list_add_before (pos->next, list);
}

JOO_END_EXTERN

#endif //INCLUDE_JOO_LIST_H

// vim:set et sw=2 sts=2:
