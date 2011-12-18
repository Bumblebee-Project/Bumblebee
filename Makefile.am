# -*- Makefile -*-

AM_CPPFLAGS = ${regular_CPPFLAGS} ${libX11_CFLAGS} \
		-DCONFIG_FILE='"$(sysconfdir)/bumblebee/bumblebee.conf"' \
		-DCONF_XORG='"$(sysconfdir)/bumblebee/xorg.conf.DRIVER"'
AM_CFLAGS = ${regular_CFLAGS} -Wextra -funsigned-char -DGITVERSION="\"`git describe --tags`\""

bin_PROGRAMS = bumblebeed optirun

optirun_SOURCES = src/bbconfig.c src/bblogger.c src/bbrun.c src/bbsocket.c src/optirun.c
bumblebeed_SOURCES = src/pci.c src/bbconfig.c src/bblogger.c src/bbrun.c src/bbsocket.c src/bbsecondary.c \
	src/switch/switching.c src/switch/sw_bbswitch.c src/switch/sw_switcheroo.c src/bumblebeed.c
bumblebeed_LDADD = ${libX11_LIBS}

bumblebeedconfdir=$(sysconfdir)/bumblebee
bumblebeedconf_DATA = conf/bumblebee.conf conf/xorg.conf.nouveau conf/xorg.conf.nvidia
