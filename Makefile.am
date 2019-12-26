# -*- Makefile -*-

GITVERSION = $(shell "$(top_srcdir)/version.sh" "$(top_srcdir)")
relnotes = doc/RELEASE_NOTES_3_2_1

DISTCHECK_CONFIGURE_FLAGS = --with-udev-rules=$$dc_install_base/lib/udev/rules.d

bumblebeedconfdir=$(sysconfdir)/bumblebee

AM_CPPFLAGS = ${regular_CPPFLAGS} \
		-DCONFIG_FILE='"$(bumblebeedconfdir)/bumblebee.conf"' \
		-DCONF_XORG='"$(bumblebeedconfdir)/xorg.conf.DRIVER"' \
		-DCONF_XORG_DIR='"$(bumblebeedconfdir)/xorg.conf.d"'
AM_CFLAGS = ${regular_CFLAGS} \
		${x11_CFLAGS} ${libbsd_CFLAGS} ${glib_CFLAGS} ${kmod_CFLAGS} \
		-Wextra -funsigned-char -DGITVERSION='"${GITVERSION}"'

noinst_SCRIPTS = scripts/systemd/bumblebeed.service \
	scripts/upstart/bumblebeed.conf

if WITH_PIDFILE
noinst_SCRIPTS += scripts/sysvinit/bumblebeed
endif

bin_SCRIPTS = scripts/bumblebee-bugreport

xconfddir = $(bumblebeedconfdir)/xorg.conf.d
xconfd_DATA = conf/xorg.conf.d/10-dummy.conf

CLEANFILES = $(noinst_SCRIPTS) conf/bumblebee.conf $(bin_SCRIPTS)
EXTRA_DIST = scripts/systemd/bumblebeed.service.in \
	scripts/upstart/bumblebeed.conf.in \
	conf/99-bumblebee-nvidia-dev.rules \
	conf/bumblebee.conf.in \
	conf/xorg.conf.nvidia \
	conf/xorg.conf.nouveau \
	$(xconfd_DATA) \
	README.markdown \
	scripts/bash_completion/optirun \
	scripts/bumblebee-bugreport.in \
	$(relnotes) \
	version.sh \
	src/bbconfig.h \
	src/bblogger.h \
	src/bbrun.h \
	src/bbsecondary.h \
	src/bbsocketclient.h \
	src/bbsocket.h \
	src/driver.h \
	src/module.h \
	src/pci.h \
	src/switch/switching.h

if WITH_PIDFILE
EXTRA_DIST += scripts/sysvinit/bumblebeed.in
endif

sbin_PROGRAMS = bin/bumblebeed
bin_PROGRAMS = bin/optirun

bin_optirun_SOURCES = src/module.c src/bbconfig.c src/bblogger.c src/bbrun.c \
	src/bbsocket.c src/optirun.c src/bbsocketclient.c
bin_optirun_LDADD = ${glib_LIBS} ${kmod_LIBS} -lrt
bin_bumblebeed_SOURCES = src/pci.c src/bbconfig.c src/bblogger.c src/bbrun.c \
	src/bbsocket.c src/module.c src/bbsecondary.c src/switch/switching.c \
	src/switch/sw_bbswitch.c src/switch/sw_switcheroo.c \
	src/driver.c src/bumblebeed.c
bin_bumblebeed_LDADD = ${x11_LIBS} ${libbsd_LIBS} ${glib_LIBS} ${kmod_LIBS} -lrt

dist_doc_DATA = $(relnotes) README.markdown
bumblebeedconf_DATA = conf/bumblebee.conf conf/xorg.conf.nouveau conf/xorg.conf.nvidia

completiondir = $(sysconfdir)/bash_completion.d
completion_DATA = scripts/bash_completion/optirun

if WITH_UDEV_RULES
udevrulesdir = $(UDEV_RULES_DIR)
udevrules_DATA = conf/99-bumblebee-nvidia-dev.rules
endif

do_subst = sed -e 's|[@]GITVERSION[@]|$(GITVERSION)|g' \
	-e 's|[@]CONF_XDISP[@]|$(CONF_XDISP)|g' \
	-e 's|[@]CONF_XORG_BINARY[@]|$(CONF_XORG_BINARY)|g' \
	-e 's|[@]CONF_SOCKPATH[@]|$(CONF_SOCKPATH)|g' \
	-e 's|[@]CONF_GID[@]|$(CONF_GID)|g' \
	-e 's|[@]CONF_PM_METHOD[@]|$(CONF_PM_METHOD)|g' \
	-e 's|[@]CONF_KEEPONEXIT[@]|$(CONF_KEEPONEXIT)|g' \
	-e 's|[@]CONF_FALLBACKSTART[@]|$(CONF_FALLBACKSTART)|g' \
	-e 's|[@]CONF_BRIDGE[@]|$(CONF_BRIDGE)|g' \
	-e 's|[@]CONF_VGLCOMPRESS[@]|$(CONF_VGLCOMPRESS)|g' \
	-e 's|[@]CONF_PRIMUS_LD_PATH[@]|$(CONF_PRIMUS_LD_PATH)|g' \
	-e 's|[@]CONF_DRIVER[@]|$(CONF_DRIVER)|g' \
	-e 's|[@]CONF_TURNOFFATEXIT[@]|$(CONF_TURNOFFATEXIT)|g' \
	-e 's|[@]BINDIR[@]|$(bindir)|g' \
	-e 's|[@]SBINDIR[@]|$(sbindir)|g' \
	-e 's|[@]BBCONFDIR[@]|$(bumblebeedconfdir)|g' \
	-e 's|[@]XCONFDDIR[@]|$(xconfddir)|g' \
	-e 's|[@]CONF_DRIVER_MODULE_NVIDIA[@]|$(CONF_DRIVER_MODULE_NVIDIA)|g' \
	-e 's|[@]CONF_LDPATH_NVIDIA[@]|$(CONF_LDPATH_NVIDIA)|g' \
	-e 's|[@]CONF_GLXNAME_NVIDIA[@]|$(CONF_GLXNAME_NVIDIA)|g' \
	-e 's|[@]CONF_MODPATH_NVIDIA[@]|$(CONF_MODPATH_NVIDIA)|g' \
	-e 's|[@]CONF_PIDFILE[@]|$(CONF_PIDFILE)|g'

conf/bumblebee.conf: $(srcdir)/conf/bumblebee.conf.in
	mkdir -p conf
	$(do_subst) < $< > $@

scripts/systemd/bumblebeed.service: $(srcdir)/scripts/systemd/bumblebeed.service.in
	mkdir -p scripts/systemd
	$(do_subst) < $< > $@

scripts/bumblebee-bugreport: $(srcdir)/scripts/bumblebee-bugreport.in
	mkdir -p scripts
	$(do_subst) < $< > $@

if WITH_PIDFILE
scripts/sysvinit/bumblebeed: $(srcdir)/scripts/sysvinit/bumblebeed.in
	mkdir -p scripts/sysvinit
	$(do_subst) < $< > $@
endif

scripts/upstart/bumblebeed.conf: $(srcdir)/scripts/upstart/bumblebeed.conf.in
	mkdir -p scripts/upstart
	$(do_subst) < $< > $@

if HAVE_HELP2MAN
man_MANS = bumblebeed.1 optirun.1
CLEANFILES += $(man_MANS)

optirun.1: bin/optirun $(bin_optirun_SOURCES)
	$(HELP2MAN) --output=$@ --no-info \
		--name='run programs on the discrete video card' \
		bin/optirun

bumblebeed.1: bin/bumblebeed $(bin_bumblebeed_SOURCES)
	$(HELP2MAN) --output=$@ --no-info \
		--name='bumblebee daemon' \
		bin/bumblebeed
else
	@echo "Warning: help2man not available, no man page is created."
endif

dist-hook:
	echo $(PACKAGE_VERSION) > $(distdir)/VERSION
