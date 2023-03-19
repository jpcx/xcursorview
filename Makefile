# xcursorview
# Copyright (C) 2023 Justin Collier <m@jpcx.dev>
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

PROGRAM   = xcursorview
LIBRARIES = xi x11 xfixes xext
CFLAGS    += $(foreach lib,$(LIBRARIES),$(shell pkg-config --cflags $(lib))) \
             -O3 -Wall -Werror -pedantic
LDFLAGS   += $(foreach lib,$(LIBRARIES),$(shell pkg-config --libs $(lib)))
PREFIX    ?= /usr/local
BINDIR    ?= $(PREFIX)/bin
MANDIR    ?= $(PREFIX)/share/man/man1

${PROGRAM}: main.c
	${CC} ${CFLAGS} -o $@ $< ${LDFLAGS}

install: ${PROGRAM}
	install -d ${DESTDIR}${BINDIR}
	install -d ${DESTDIR}${MANDIR}
	install -m 0755 ${PROGRAM} ${DESTDIR}${BINDIR}/${PROGRAM}
	install -m 0644 ${PROGRAM}.1 ${DESTDIR}${MANDIR}/${PROGRAM}.1

uninstall:
	rm -f ${DESTDIR}${BINDIR}/${PROGRAM}
	rm -f ${DESTDIR}${MANDIR}/${PROGRAM}.1

clean:
	${RM} ${PROGRAM}

.PHONY: install clean
