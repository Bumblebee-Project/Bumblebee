#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([bumblebeed], [bleeding-edge], [https://github.com/Bumblebee-Project/bumblebeed])
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

default_pidfile=/var/run/bumblebeed.pid
AC_DEFINE_CONF(CONF_PIDFILE, [Path to the pidfile or $default_pidfile if empty], [
if test "x" = "x$CONF_PIDFILE"; then
	CONF_PIDFILE="$default_pidfile"
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
	tar-pax no-dist-gzip dist-xz])

AC_ARG_WITH([pidfile],
	[AS_HELP_STRING([--without-pidfile],
	[support for pidfiles in the daemon @<:@default=yes@:>@])],
	[],
	[with_pidfile=yes])
AS_IF([test "x$with_pidfile" != xno],
	[AC_DEFINE(WITH_PIDFILE, [1], [Define if pidfiles should be available])
	])
AM_CONDITIONAL([WITH_PIDFILE], [test "x$with_pidfile" != xno])

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
#AC_CHECK_LIB([bsd], [pidfile_open])

# Checks for header files.
PKG_CHECK_MODULES([x11], [x11])
PKG_CHECK_MODULES([glib], [glib-2.0])
AS_IF([test "x$with_pidfile" != xno],
	[PKG_CHECK_MODULES([libbsd], [libbsd >= 0.2.0])])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
