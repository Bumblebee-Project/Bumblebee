#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([bumblebeed], [bleeding-edge], [https://github.com/Bumblebee-Project/bumblebeed])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([src/bbsocket.c])
AC_CONFIG_HEADERS([config.h])

AC_DEFINE_UNQUOTED(GITVERSION, "`git describe --tags`", "git version number")

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
