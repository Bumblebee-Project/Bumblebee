# -*- Makefile -*-

AM_CPPFLAGS = ${regular_CPPFLAGS} ${libX11_CFLAGS}
AM_CFLAGS = ${regular_CFLAGS} -Wextra -funsigned-char -DGITVERSION="\"`git describe --tags`\""

bin_PROGRAMS = bin/bumblebeed bin/optirun

bin_optirun_SOURCES = src/module.c src/bbconfig.c src/bblogger.c src/bbrun.c src/bbsocket.c src/optirun.c
bin_bumblebeed_SOURCES = src/pci.c src/bbconfig.c src/bblogger.c src/bbrun.c src/bbsocket.c src/module.c src/bbsecondary.c \
	src/switch/switching.c src/switch/sw_bbswitch.c src/switch/sw_switcheroo.c src/bumblebeed.c
bin_bumblebeed_LDADD = ${libX11_LIBS}

bumblebeedconfdir=$(sysconfdir)/bumblebee
bumblebeedconf_DATA = conf/bumblebee.conf conf/xorg.conf.nouveau conf/xorg.conf.nvidia
