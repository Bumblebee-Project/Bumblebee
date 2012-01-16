# -*- Makefile -*-

GITVERSION = $(shell "$(top_srcdir)/version.sh" "$(top_srcdir)")

bumblebeedconfdir=$(sysconfdir)/bumblebee

AM_CPPFLAGS = ${regular_CPPFLAGS} \
		-DCONFIG_FILE='"$(bumblebeedconfdir)/bumblebee.conf"' \
		-DCONF_XORG='"$(bumblebeedconfdir)/xorg.conf.DRIVER"'
AM_CFLAGS = ${regular_CFLAGS} \
		${x11_CFLAGS} ${libbsd_CFLAGS} ${glib_CFLAGS} \
		-Wextra -funsigned-char -DGITVERSION='"${GITVERSION}"'

noinst_SCRIPTS = scripts/systemd/bumblebeed.service \
	scripts/upstart/bumblebeed.conf

if WITH_PIDFILE
noinst_SCRIPTS += scripts/sysvinit/bumblebeed
endif

dist_bin_SCRIPTS = scripts/bumblebee-bugreport

CLEANFILES = $(noinst_SCRIPTS) conf/bumblebee.conf
EXTRA_DIST = scripts/systemd/bumblebeed.service.in \
	scripts/upstart/bumblebeed.conf.in \
	conf/bumblebee.conf.in \
	conf/xorg.conf.nvidia \
	conf/xorg.conf.nouveau \
	README.markdown doc/daemon.notes \
	scripts/bash_completion/bumblebee \
	version.sh
# for laziness include all headers found
EXTRA_DIST += src/*.h src/*/switching.h

if WITH_PIDFILE
EXTRA_DIST += scripts/sysvinit/bumblebeed.in
endif

bin_PROGRAMS = bin/bumblebeed bin/optirun

bin_optirun_SOURCES = src/module.c src/bbconfig.c src/bblogger.c src/bbrun.c \
	src/bbsocket.c src/driver.c src/optirun.c
bin_optirun_LDADD = ${glib_LIBS}
bin_bumblebeed_SOURCES = src/pci.c src/bbconfig.c src/bblogger.c src/bbrun.c \
	src/bbsocket.c src/module.c src/bbsecondary.c src/switch/switching.c \
	src/switch/sw_bbswitch.c src/switch/sw_switcheroo.c \
	src/driver.c src/bumblebeed.c
bin_bumblebeed_LDADD = ${x11_LIBS} ${libbsd_LIBS} ${glib_LIBS}

bumblebeedconf_DATA = conf/bumblebee.conf conf/xorg.conf.nouveau conf/xorg.conf.nvidia

completiondir = $(sysconfdir)/bash_completion.d
completion_DATA = scripts/bash_completion/bumblebee

do_subst = sed -e 's|[@]GITVERSION[@]|$(GITVERSION)|g' \
	-e 's|[@]CONF_XDISP[@]|$(CONF_XDISP)|g' \
	-e 's|[@]CONF_SOCKPATH[@]|$(CONF_SOCKPATH)|g' \
	-e 's|[@]CONF_GID[@]|$(CONF_GID)|g' \
	-e 's|[@]CONF_PM_METHOD[@]|$(CONF_PM_METHOD)|g' \
	-e 's|[@]CONF_KEEPONEXIT[@]|$(CONF_KEEPONEXIT)|g' \
	-e 's|[@]CONF_FALLBACKSTART[@]|$(CONF_FALLBACKSTART)|g' \
	-e 's|[@]CONF_VGLCOMPRESS[@]|$(CONF_VGLCOMPRESS)|g' \
	-e 's|[@]CONF_DRIVER[@]|$(CONF_DRIVER)|g' \
	-e 's|[@]CONF_TURNOFFATEXIT[@]|$(CONF_TURNOFFATEXIT)|g' \
	-e 's|[@]BINDIR[@]|$(bindir)|g' \
	-e 's|[@]BBCONFDIR[@]|$(bumblebeedconfdir)|g' \
	-e 's|[@]CONF_XORG[@]|$(bumblebeedconfdir)/xorg.conf.DRIVER|g' \
	-e 's|[@]CONF_DRIVER_MODULE_NVIDIA[@]|$(CONF_DRIVER_MODULE_NVIDIA)|g' \
	-e 's|[@]CONF_LDPATH_NVIDIA[@]|$(CONF_LDPATH_NVIDIA)|g' \
	-e 's|[@]CONF_MODPATH_NVIDIA[@]|$(CONF_MODPATH_NVIDIA)|g' \
	-e 's|[@]CONF_PIDFILE[@]|$(CONF_PIDFILE)|g'

conf/bumblebee.conf: conf/bumblebee.conf.in
	$(do_subst) < $@.in > $@

scripts/systemd/bumblebeed.service: scripts/systemd/bumblebeed.service.in
	$(do_subst) < $@.in > $@

scripts/bumblebee-bugreport: scripts/bumblebee-bugreport.in
	$(do_subst) < $@.in > $@

if WITH_PIDFILE
scripts/sysvinit/bumblebeed: scripts/sysvinit/bumblebeed.in
	$(do_subst) < $@.in > $@
endif

scripts/upstart/bumblebeed.conf: scripts/upstart/bumblebeed.conf.in
	$(do_subst) < $@.in > $@

if HAVE_HELP2MAN
man_MANS = doc/bumblebeed.1 doc/optirun.1
CLEANFILES += $(man_MANS)

doc/optirun.1: $(bin_PROGRAMS) $(bin_optirun_SOURCES)
	$(HELP2MAN) --output=$@ --no-info \
		--name='run programs on the discrete video card' \
		bin/optirun

doc/bumblebeed.1: $(bin_PROGRAMS) $(bin_bumblebeed_SOURCES)
	$(HELP2MAN) --output=$@ --no-info \
		--name='bumblebee daemon' \
		bin/bumblebeed
else
	@echo "Warning: help2man not available, no man page is created."
endif

dist-hook:
	echo $(PACKAGE_VERSION) > $(distdir)/VERSION
