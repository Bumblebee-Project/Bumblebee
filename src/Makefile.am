# -*- Makefile -*

AM_CPPFLAGS = -DVERSION=${VERSION} ${libX11_CFLAGS}
AM_CFLAGS = -Wall -Wextra -funsigned-char

bin_PROGRAMS = bumblebeed

bumblebeed_SOURCES = bbconfig.c bbglobals.c bblogger.c bbrun.c \
	bbsocket.c bbswitch.c bumblebeed.c
bumblebeed_LDADD = ${libX11_LIBS}
