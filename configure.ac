#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([bumblebeed], [0.1], [https://github.com/Bumblebee-Project/bumblebeed])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([src/bbsocket.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([1.10.2 -Wall foreign subdir-objects
	tar-pax no-dist-gzip dist-xz])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

# Checks for libraries.

# Checks for header files.
PKG_CHECK_MODULES([libX11], [x11])
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([socket strerror])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
