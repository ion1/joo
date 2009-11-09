# joo – a wrapper for ei
#
# Copyright © 2009 Johan Kiviniemi
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

CFLAGS  := -fPIC -g -Os -W -Wall -Werror -std=gnu99 -I.
LDFLAGS := -Wl,--as-needed -Ljoo
LDLIBS  := -lei

LN := ln -f

all_sources := $(wildcard joo/*.c example/*.c)
all_objects := $(all_sources:.c=.o)
all_deps    := $(all_sources:.c=.dep)

all : joo/libjoo.so example/reader example/writer

clean :=

joo/libjoo.so.0.0.1 : LDFLAGS += -Wl,-soname,libjoo.so.0
joo/libjoo.so.0.0.1 : joo/matcher.o joo/reader.o joo/term.o joo/writer.o
	$(CC) -shared $(LDFLAGS) -o $@ $^ $(LDLIBS)
clean += joo/libjoo.so.0.0.1

joo/libjoo.so.0 : joo/libjoo.so.0.0.1
	$(LN) -s $(notdir $^) $@
clean += joo/libjoo.so.0

joo/libjoo.so : joo/libjoo.so.0
	$(LN) -s $(notdir $^) $@
clean += joo/libjoo.so

example/reader : LDLIBS += -ljoo
example/reader : example/reader.o joo/libjoo.so
clean += example/reader

example/writer : LDLIBS += -ljoo
example/writer : example/writer.o joo/libjoo.so
clean += example/writer

.PHONY : clean
clean ::
	$(RM) $(clean) $(all_objects) $(all_deps)

-include $(all_deps)

%.dep : %.c
	$(CC) -MM -MQ $(<:.c=.o) $(CFLAGS) -o $@ $<

