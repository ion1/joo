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
#include <unistd.h>  // STD*_FILENO

#include <joo/matcher.h>
#include <joo/reader.h>

static int
no_match (void *user_data)
{
  assert ((int)user_data == 1);

  printf ("no_match\n");

  return -1;
}

static int
o_hai (void *user_data, JooPushedData *pushed_data)
{
  JooPushedData *str;

  assert ((int)user_data == 2);

  str = (JooPushedData *)pushed_data->entry.next;
  assert (str != pushed_data);

  printf ("o_hai: %s\n", str->data);

  return 0;
}

int
main (void)
{
  JooMatcher *matcher;

  assert (matcher = joo_matcher_new (no_match, (void *)1));

  joo_matcher_add (matcher, o_hai, (void *)2, (JooTerm[]){
    joo_tuple_start (),
    joo_atom ("o_hai"),
    joo_push_string (),
    joo_tuple_end (),
    joo_end () });

  joo_reader_init (STDIN_FILENO, matcher);
  joo_reader_main_loop ();

  joo_matcher_free (matcher);

  return 0;
}

// vim:set et sw=2 sts=2:
