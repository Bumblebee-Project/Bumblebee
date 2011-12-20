#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([bumblebeed], [bleeding-edge], [https://github.com/Bumblebee-Project/bumblebeed])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([src/bbsocket.c])
AC_CONFIG_HEADERS([config.h])

# version number
AC_DEFINE_UNQUOTED(GITVERSION, "`git describe --tags`", "git version number")

# default configuration
AC_DEFINE(CONF_XDISP, ":8", "X screen number")
AC_DEFINE(CONF_XORG, "/etc/bumblebee/xorg.conf.%s", "xorg.conf path, %s will be replaced by used driver")
AC_DEFINE(CONFIG_FILE, "/etc/bumblebee/bumblebeed.conf", "configuration file path")
AC_DEFINE(CONF_LDPATH, "/usr/lib64/nvidia-current", "ld path")
AC_DEFINE(CONF_SOCKPATH, "/var/run/bumblebee.socket", "socket path")
AC_DEFINE(CONF_GID, "bumblebee", "group to use when setting GID")
AC_DEFINE(CONF_PMENABLE, 1, "enable power management")
AC_DEFINE(CONF_STOPONEXIT, 1, "stop secondary X on exit of last optirun executable")
AC_DEFINE(CONF_FALLBACKSTART, 1, "make optirun start applications normally if secondary is unavailable")
AC_DEFINE(CONF_VGLCOMPRESS, "proxy", "vglclient transport method")
AC_DEFINE(CONF_DRIVER, "nouveau", "nvidia driver to use")
AC_DEFINE(CONF_SHUTDOWNSTATE, 1, "state of card when shutting off daemon")

# non-config related compile-time defines
AC_DEFINE(DAEMON_NAME, "bumblebeed", "daemon name")

AM_INIT_AUTOMAKE([1.10.2 -Wall foreign subdir-objects
	tar-pax no-dist-gzip dist-xz])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

regular_CPPFLAGS="-D_FILE_OFFSET_BITS=64 -D_REENTRANT"
regular_CFLAGS="-Wall -Waggregate-return \
	-Wmissing-declarations -Wmissing-prototypes -Wredundant-decls \
	-Wshadow -Wstrict-prototypes -Winline -Wformat=2 -pipe"
AC_SUBST([regular_CPPFLAGS])
AC_SUBST([regular_CFLAGS])

# Checks for libraries.

# Checks for header files.
PKG_CHECK_MODULES([libX11], [x11])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
