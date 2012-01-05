# -*- Makefile -*-

AM_CPPFLAGS = ${regular_CPPFLAGS} ${libX11_CFLAGS} \
		-DCONFIG_FILE='"$(sysconfdir)/bumblebee/bumblebee.conf"' \
		-DCONF_XORG='"$(sysconfdir)/bumblebee/xorg.conf.DRIVER"'
AM_CFLAGS = ${regular_CFLAGS} -Wextra -funsigned-char -DGITVERSION="\"`git describe --tags`\""

CLEANFILES = scripts/systemd/bumblebeed.service conf/bumblebee.conf
EXTRA_DIST = scripts/systemd/bumblebeed.service.in conf/bumblebee.conf.in

bin_PROGRAMS = bin/bumblebeed bin/optirun

bin_optirun_SOURCES = src/module.c src/bbconfig.c src/bblogger.c src/bbrun.c src/bbsocket.c src/optirun.c
bin_bumblebeed_SOURCES = src/pci.c src/bbconfig.c src/bblogger.c src/bbrun.c src/bbsocket.c src/module.c src/bbsecondary.c \
	src/switch/switching.c src/switch/sw_bbswitch.c src/switch/sw_switcheroo.c src/bumblebeed.c
bin_bumblebeed_LDADD = ${libX11_LIBS}

bumblebeedconfdir=$(sysconfdir)/bumblebee
bumblebeedconf_DATA = conf/bumblebee.conf conf/xorg.conf.nouveau conf/xorg.conf.nvidia

do_subst = sed -e 's|[@]CONF_XDISP[@]|$(CONF_XDISP)|g' \
	-e 's|[@]CONF_LDPATH[@]|$(CONF_LDPATH)|g' \
	-e 's|[@]CONF_MODPATH[@]|$(CONF_MODPATH)|g' \
	-e 's|[@]CONF_SOCKPATH[@]|$(CONF_SOCKPATH)|g' \
	-e 's|[@]CONF_GID[@]|$(CONF_GID)|g' \
	-e 's|[@]CONF_PM_METHOD[@]|$(CONF_PM_METHOD)|g' \
	-e 's|[@]CONF_STOPONEXIT[@]|$(CONF_STOPONEXIT)|g' \
	-e 's|[@]CONF_FALLBACKSTART[@]|$(CONF_FALLBACKSTART)|g' \
	-e 's|[@]CONF_VGLCOMPRESS[@]|$(CONF_VGLCOMPRESS)|g' \
	-e 's|[@]CONF_DRIVER[@]|$(CONF_DRIVER)|g' \
	-e 's|[@]CONF_DRIVER_MODULE[@]|$(CONF_DRIVER_MODULE)|g' \
	-e 's|[@]CONF_SHUTDOWNSTATE[@]|$(CONF_SHUTDOWNSTATE)|g' \
	-e 's|[@]BINDIR[@]|$(bindir)|g' \
	-e 's|[@]CONF_XORG[@]|$(sysconfdir)/bumblebee/xorg.conf.DRIVER|g'

conf/bumblebee.conf: conf/bumblebee.conf.in
	$(do_subst) < $@.in > $@

scripts/systemd/bumblebeed.service: scripts/systemd/bumblebeed.service.in
	$(do_subst) < $@.in > $@
