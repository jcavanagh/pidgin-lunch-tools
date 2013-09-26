LIBDIR ?= ~/.purple/plugins
DATADIR ?= /usr/share
CC = gcc

LIBPURPLE_CFLAGS = -I/usr/include/libpurple -DPURPLE_PLUGINS -DENABLE_NLS
GLIB_CFLAGS = $(shell pkg-config --cflags glib-2.0)

SOURCES =     \
    src/pidgin-lunch_tools.c

all:	release

ifdef DESTDIR

install:
	install -d -m 0755    ${DESTDIR}${LIBDIR}
	install pidgin-lunch_tools.so     ${DESTDIR}${LIBDIR}

uninstall:
	rm -f ${DESTDIR}${LIBDIR}/pidgin-lunch_tools.so

else

install:
	install -d -m 0755    ${LIBDIR}
	install pidgin-lunch_tools.so     ${LIBDIR}

uninstall:
	rm -f ${LIBDIR}/pidgin-lunch_tools.so

endif

clean:
	rm -f pidgin-lunch_tools.so

pidgin-lunch_tools.so:	${SOURCES}
	${CC} ${LIBPURPLE_CFLAGS} -Wall -pthread ${GLIB_CFLAGS} -I. -g3 -O2 -pipe ${SOURCES} -o pidgin-lunch_tools.so -shared -fPIC -DPIC

release:	pidgin-lunch_tools.so