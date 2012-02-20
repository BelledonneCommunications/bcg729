#   -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.



AC_INIT([bcg729],[0.1])
AC_PREREQ(2.63)
AC_CONFIG_SRCDIR([src/encoder.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([tar-ustar --warnings=no-portability])
AC_PROG_LIBTOOL
AC_PROG_CC
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_ARG_ENABLE(strict,
[  --enable-strict       Enable error on compilation warning [default=yes]],
[wall_werror=$enableval],
[wall_werror=yes]
)
# configure option to disable the tests
AC_ARG_ENABLE([tests],
	AS_HELP_STRING([--disable-tests], [Disable the tests]))
	AM_CONDITIONAL([RUN_TESTS], [test "x$enable_tests" != "xno"])

CFLAGS="$CFLAGS -Wall"

if test $GCC = yes && test $wall_werror = yes;  then
	CFLAGS="$CFLAGS -Werror "
fi

# check for libraries that have pkg-config files installed
PKG_CHECK_MODULES(ORTP, ortp >= 0.16.0,[found_ortp=true],foo=bar)
PKG_CHECK_MODULES(MEDIASTREAMER, mediastreamer >= 2.0.0,[found_ms2=true],foo=bar)
AM_CONDITIONAL(BUILD_MSBCG729, test x$found_ms2 = xtrue && test x$found_ortp = xtrue)

# Create the following files from their .in counterparts
AC_CONFIG_FILES([
  Makefile
  src/Makefile 
  include/Makefile 
  include/bcg729/Makefile 
  msbcg729/Makefile 
  test/Makefile 
  test/bin/Makefile 
  libbcg729.pc
])

AC_OUTPUT
