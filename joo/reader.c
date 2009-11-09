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
#include <fcntl.h>
#include <stdio.h>   // NULL
#include <stdlib.h>  // realloc
#include <string.h>  // memcpy, memmove
#include <sys/select.h>
#include <unistd.h>  // read

#include "matcher.h"

#define BUF_SIZE 512

static int         reader_fd = -1;
static JooMatcher *reader_matcher = NULL;

static char       *reader_buf = NULL;
static ssize_t     reader_buf_size = -1;
static ssize_t     reader_buf_pos = 0;

#define max(a, b) ({ \
  typeof (a) a_val = (a); \
  typeof (b) b_val = (b); \
  a_val > b_val ? a_val : b_val; })

static void
deinit (void)
{
  if (reader_buf)
    free (reader_buf);
  reader_buf = NULL;
  reader_buf_size = -1;
  reader_buf_pos = 0;

  reader_fd = -1;
  reader_matcher = NULL;
}

static int
handle_buf (const char *buf,
            int         buf_size)
{
  ssize_t new_size = (reader_buf_size < 0 ? BUF_SIZE : reader_buf_size);
  while (new_size < reader_buf_pos + buf_size)
    new_size *= 1.5;

  if (new_size != reader_buf_size) {
    char *new_buf = realloc (reader_buf, new_size);
    if (! new_buf) {
      deinit ();
      return -1;
    }
    reader_buf = new_buf;
  }

  memcpy (reader_buf+reader_buf_pos, buf, buf_size);
  reader_buf_pos += buf_size;

  for (;;) {
    ssize_t cmd_size, take_size;

    if (reader_buf_pos < 2)
      break;

    cmd_size = (reader_buf[0] << 8) + reader_buf[1];
    take_size = 2+cmd_size;

    if (take_size > reader_buf_pos)
      break;

    joo_matcher_match (reader_matcher, reader_buf+2, cmd_size);
    memmove (reader_buf, reader_buf+take_size, reader_buf_pos-take_size);
    reader_buf_pos -= take_size;
  }

  return 0;
}

int
joo_reader_init (int         fd,
                 JooMatcher *matcher)
{
  int flags;

  assert (fd >= 0);
  assert (matcher != NULL);

  flags = fcntl (fd, F_GETFL);
  if (flags < 0)
    return -1;

  if (fcntl (fd, F_SETFL, flags|O_NONBLOCK) < 0)
    return -1;

  reader_fd = fd;
  reader_matcher = matcher;

  return 0;
}

int
joo_reader_read (void)
{
  ssize_t res;
  char    buf[BUF_SIZE];

  assert (reader_fd >= 0);
  assert (reader_matcher != NULL);

  res = read (reader_fd, buf, BUF_SIZE);
  if (res == 0) {
    deinit ();
    return -1;
  } else if ((res < 0) && (errno != EAGAIN) && (errno != EWOULDBLOCK)) {
    deinit ();
    return -1;
  }

  return handle_buf (buf, res);
}

int
joo_reader_main_loop (void)
{
  assert (reader_fd >= 0);
  assert (reader_matcher != NULL);

  while (reader_fd >= 0) {
    fd_set readfds;
    int    nfds;
    int    res;

    FD_ZERO (&readfds);
    nfds = 0;

    FD_SET (reader_fd, &readfds);
    nfds = max (nfds, reader_fd+1);

    res = select (nfds, &readfds, NULL, NULL, NULL);
    if (res < 0)
      return -1;

    if (res > 0)
      if (joo_reader_read () < 0)
        break;
  }

  return 0;
}

// vim:set et sw=2 sts=2:
