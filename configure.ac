#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([bumblebee], [3.2.1])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([src/bbsocket.c])
#AC_CONFIG_HEADERS([config.h])

# AC_DEFINE_SUBST(NAME, VALUE, DESCRIPTION)
# -----------------------------------------
AC_DEFUN([AC_DEFINE_SUBST], [
AC_SUBST([$1], [$2])
AC_DEFINE_UNQUOTED([$1], "$[$1]", [$3])
])

# AC_DEFINE_CONF(NAME, HELP_TEXT, VAR_SET_CODE)
# --------------------------------------
AC_DEFUN([AC_DEFINE_CONF], [
AC_ARG_VAR([$1], [$2])
$3
AC_DEFINE_UNQUOTED([$1], "$[$1]", [$2])
])

# default configuration
# Don't be stupid by using values like "path/$(nuke_machine)", it gets expanded!
AC_DEFINE_SUBST(CONF_XDISP, ":8", [X screen number])
AC_DEFINE_SUBST(CONF_SOCKPATH, "/var/run/bumblebee.socket", [socket path])
AC_DEFINE_SUBST(CONF_GID, "bumblebee", [group to use when setting GID])
AC_DEFINE_SUBST(CONF_KEEPONEXIT, "false", [stop secondary X on exit of last optirun executable])
AC_DEFINE_SUBST(CONF_FALLBACKSTART, "false", [make optirun start applications normally if secondary is unavailable])
AC_DEFINE_SUBST(CONF_VGLCOMPRESS, "proxy", [vglclient transport method])
AC_DEFINE_SUBST(CONF_TURNOFFATEXIT, "false", [state of card when shutting off daemon])

# OpenSUSE: /usr/bin/X -> /var/lib/X11/X -> /usr/bin/Xorg
# Fedora, Arch Linux: /usr/bin/X -> /usr/bin/Xorg
# Ubuntu: /usr/bin/X is a custom binary doing authorization and then executes
#         /etc/X11/X -> /usr/bin/Xorg
# Debian/Ubuntu and Xorg >= 1.18: /usr/bin/Xorg is a script that will run either the normal
#                                 Xorg binary from /usr/lib/xorg/Xorg, or a SETUID
#                                 wrapper at /usr/lib/xorg/Xorg.wrap
#                                 Use this setting to run /usr/lib/xorg/Xorg specifically
#                                 in that case.
AC_DEFINE_CONF(CONF_XORG_BINARY, [Xorg binary to run], [
if test "x" = "x$CONF_XORG_BINARY"; then
	CONF_XORG_BINARY=Xorg
fi
])

AC_DEFINE_CONF(CONF_BRIDGE, [optirun display/render bridge, valid values are auto (default), primus and virtualgl], [
case $CONF_BRIDGE in
auto|primus|virtualgl) ;;
"") CONF_BRIDGE=auto ;;
*) AC_MSG_ERROR([Invalid value for CONF_BRIDGE]) ;;
esac
])

AC_DEFINE_CONF(CONF_PRIMUS_LD_PATH, [List of paths which are searched for the primus libGL.so.1 when using the primus bridge], [
if test "x" = "x$CONF_PRIMUS_LD_PATH"; then
	CONF_PRIMUS_LD_PATH="/usr/lib/primus:/usr/lib32/primus"
fi
])

AC_DEFINE_CONF(CONF_PIDFILE, [Path to the pidfile or /var/run/bumblebeed.pid if empty], [
if test "x" = "x$CONF_PIDFILE"; then
	CONF_PIDFILE=/var/run/bumblebeed.pid
fi
])

AC_DEFINE_CONF(CONF_PM_METHOD, [Power management method, valid values are auto (default), bbswitch, switcheroo and none], [
case $CONF_PM_METHOD in
auto|bbswitch|switcheroo|none) ;;
"") CONF_PM_METHOD=auto ;;
*) AC_MSG_ERROR([Invalid value for CONF_PM_METHOD]) ;;
esac
])
AC_DEFINE_CONF(CONF_DRIVER, [default driver to use, valid values are nvidia, nouveau and an empty value (for autodetection, default)], [
case $CONF_DRIVER in
nvidia|nouveau|"") ;;
*) AC_MSG_ERROR([Invalid value for CONF_DRIVER]) ;;
esac
])

AC_DEFINE_CONF(CONF_DRIVER_MODULE_NVIDIA, [name of module for nvidia driver, i.e. the filename without .ko. If empty, "nvidia" is assumed])
AC_DEFINE_CONF(CONF_LDPATH_NVIDIA, [Path to nvidia libraries for nvidia driver])
AC_DEFINE_CONF(CONF_MODPATH_NVIDIA, [ModulePath for xorg modules when using nvidia driver])

# non-config related compile-time defines
AC_DEFINE(DAEMON_NAME, "bumblebeed", "daemon name")

AM_INIT_AUTOMAKE([1.10.2 -Wall foreign subdir-objects
	tar-pax])

AC_ARG_WITH([pidfile],
	[AS_HELP_STRING([--without-pidfile],
	[support for pidfiles in the daemon @<:@default=yes@:>@])],
	[],
	[with_pidfile=yes])
AS_IF([test "x$with_pidfile" != xno],
	[AC_DEFINE(WITH_PIDFILE, [1], [Define if pidfiles should be available])
	])
AM_CONDITIONAL([WITH_PIDFILE], [test "x$with_pidfile" != xno])

UDEV_RULES_DIR=/lib/udev/rules.d
AC_ARG_WITH([udev-rules],
	[AS_HELP_STRING([--with-udev-rules@<:@=DIR@:>@],
	[install nvidia udev rule @<:@default=/lib/udev/rules.d@:>@])],
	[],
	[with_udev_rules=yes])
AS_CASE([$with_udev_rules],
	[no], [UDEV_RULES_DIR=],
	[yes], [], # --with-udev-rules: keep the default
	[UDEV_RULES_DIR=$with_udev_rules])
AC_SUBST([UDEV_RULES_DIR])
AM_CONDITIONAL([WITH_UDEV_RULES], [test -n "$UDEV_RULES_DIR"])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PATH_PROG([HELP2MAN], help2man)
AM_CONDITIONAL([HAVE_HELP2MAN], [test -n "$HELP2MAN"])
if test x"$HELP2MAN" = x; then
	AC_MSG_WARN([cannot find help2man, you will not be able to generate
		manpages])
fi

regular_CPPFLAGS="-D_FILE_OFFSET_BITS=64 -D_REENTRANT"
regular_CFLAGS="-Wall -Waggregate-return \
	-Wmissing-declarations -Wmissing-prototypes -Wredundant-decls \
	-Wshadow -Wstrict-prototypes -Winline -Wformat=2 -pipe"
AC_SUBST([regular_CPPFLAGS])
AC_SUBST([regular_CFLAGS])

# Checks for libraries.
#AC_CHECK_LIB([bsd], [pidfile_open])

# Checks for header files.
PKG_CHECK_MODULES([x11], [x11])
PKG_CHECK_MODULES([glib], [glib-2.0])
AS_IF([test "x$with_pidfile" != xno], [
		PKG_CHECK_MODULES([libbsd], [libbsd >= 0.2.0])
		PKG_CHECK_EXISTS([libbsd = 0.2.0], [AC_DEFINE(HAVE_LIBBSD_020)])
	])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
