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
AC_DEFINE_UNQUOTED([$1], [$2], [$3])
AC_SUBST([$1], [$2])
])

# default configuration
# Don't be stupid by using values like "path/$(nuke_machine)", it gets expanded!
AC_DEFINE_SUBST(CONF_XDISP, ":8", "X screen number")
AC_DEFINE_SUBST(CONF_LDPATH, "/usr/lib64/nvidia-current:/usr/lib/nvidia-current:/usr/lib32/nvidia-current:/usr/lib64/nvidia:/usr/lib/nvidia:/usr/lib32/nvidia", "ld path")
AC_DEFINE_SUBST(CONF_MODPATH, ["/usr/lib64/nvidia-current/xorg,/usr/lib/nvidia-current/xorg,/usr/lib32/nvidia-current/xorg,/usr/lib64/nvidia/xorg,/usr/lib/nvidia/xorg,/usr/lib32/nvidia/xorg,/usr/lib/i386-linux-gnu/xorg/extra-modules,/usr/lib/x86_64-linux-gnu/xorg/extra-modules,/usr/lib64/xorg/extra-modules,/usr/lib64/xorg/modules,/usr/lib/xorg/extra-modules,/usr/lib/xorg/modules,/usr/lib32/xorg/extra-modules,/usr/lib32/xorg/modules"], "ModulePath for xorg")
AC_DEFINE_SUBST(CONF_SOCKPATH, "/var/run/bumblebee.socket", "socket path")
AC_DEFINE_SUBST(CONF_GID, "bumblebee", "group to use when setting GID")
AC_DEFINE_SUBST(CONF_PM_METHOD, "auto", "Power management method")
AC_DEFINE_SUBST(CONF_STOPONEXIT, 1, "stop secondary X on exit of last optirun executable")
AC_DEFINE_SUBST(CONF_FALLBACKSTART, 1, "make optirun start applications normally if secondary is unavailable")
AC_DEFINE_SUBST(CONF_VGLCOMPRESS, "proxy", "vglclient transport method")
AC_DEFINE_SUBST(CONF_DRIVER, "", "nvidia driver to use (empty for autodetection)")
AC_DEFINE_SUBST(CONF_DRIVER_MODULE, "", "driver kernel module (empty defaults to autodetection)")
AC_DEFINE_SUBST(CONF_SHUTDOWNSTATE, 1, "state of card when shutting off daemon")

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
#AC_CHECK_LIB([bsd], [pidfile_open])

# Checks for header files.
PKG_CHECK_MODULES([x11], [x11])
PKG_CHECK_MODULES([glib], [glib-2.0])
PKG_CHECK_MODULES([libbsd], [libbsd >= 0.2.0])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
