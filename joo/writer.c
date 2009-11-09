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

#include <ei.h>
#include <stdlib.h>  // abort
#include <unistd.h>  // write

#include "term.h"
#include "writer.h"

static int
write_buffer (int         fd,
              const char *buf,
              ssize_t     size)
{
  ssize_t i;
  ssize_t written = 0;

  while (written < size) {
    if ((i = write (fd, buf+written, size-written)) < 0)
      return i;
    written += i;
  }

  return written;
}

static int
write_ei_x_buff (int              fd,
                 const ei_x_buff *buf)
{
  char size_header[2] = { (buf->index >> 8) & 0xff, (buf->index) & 0xff };

  if (write_buffer (fd, size_header, 2) != 2)
    return -1;

  if (write_buffer (fd, buf->buff, buf->index) != buf->index)
    return -1;

  return 0;
}

int
joo_write (int      fd,
           JooTerm *terms)
{
  int ret = 0;

  ei_x_buff buf;

  if (joo_term_count_children (terms) < 0)
    return -1;

  if (ei_x_new_with_version (&buf)) {
    errno = ENOMEM;
    return -1;
  }

  for (JooTerm *term = terms; term->type != JOO_END; term++) {
    if (term->push) {
      errno = EINVAL;
      return -1;
    }

    switch (term->type) {
      case JOO_ATOM:
        if (ei_x_encode_atom (&buf, term->value.text)) {
          errno = ENOMEM;
          return -1;
        }
        break;

      case JOO_STRING:
        if (ei_x_encode_string (&buf, term->value.text)) {
          errno = ENOMEM;
          return -1;
        }
        break;

      case JOO_BINARY:
        if (ei_x_encode_binary (&buf, term->value.binary.data,
                                term->value.binary.size)) {
          errno = ENOMEM;
          return -1;
        }
        break;

      case JOO_TUPLE_START:
        if (ei_x_encode_tuple_header (&buf, term->num_children)) {
          errno = ENOMEM;
          return -1;
        }
        break;

      case JOO_TUPLE_END:
        break;

      default:
        abort ();
    }
  }

  if (write_ei_x_buff (fd, &buf) < 0) {
    errno = EIO;
    ret = -1;
  }

  ei_x_free (&buf);

  return ret;
}


// vim:set et sw=2 sts=2:
