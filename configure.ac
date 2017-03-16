#   -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.



AC_INIT([bcg729],[1.0.3])
AC_CANONICAL_SYSTEM
AC_PREREQ(2.63)
AC_CONFIG_SRCDIR([src/encoder.c])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([tar-ustar --warnings=no-portability])
LT_INIT
AC_PROG_CC
AC_CONFIG_HEADERS(config.h)
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_ARG_ENABLE(strict,
	[  --enable-strict       Enable error on compilation warning [default=yes]],
	[wall_werror=$enableval],
	[wall_werror=yes]
)
dnl configure option to disable the tests
AC_ARG_ENABLE([tests],
	AS_HELP_STRING([--disable-tests], [Disable the tests]))
	AM_CONDITIONAL([RUN_TESTS], [test "x$enable_tests" != "xno"])

CFLAGS="$CFLAGS -Wall"

if test $GCC = yes && test $wall_werror = yes;  then
	CFLAGS="$CFLAGS -Werror "
fi

VISIBILITY_CFLAGS=
case "$target_os" in
	*mingw*)
		CFLAGS="$CFLAGS -D_WIN32_WINNT=0x0501"
		mingw_found=yes
	;;
	*)
		VISIBILITY_CFLAGS="-fvisibility=hidden"
	;;
esac
AC_SUBST(VISIBILITY_CFLAGS)

dnl Initialize libtool
LT_INIT([win32-dll shared ])

AC_CONFIG_COMMANDS([libtool-hacking],
	[if test "$mingw_found" = "yes" ; then
		echo "Hacking libtool to work with mingw..."
		sed -e 's/\*\" \$a_deplib \"\*/\*/' < ./libtool > libtool.tmp
		cp -f ./libtool.tmp ./libtool
		rm -f ./libtool.tmp
	fi],
	[mingw_found=$mingw_found]
)

dnl Create the following files from their .in counterparts
AC_CONFIG_FILES([
	Makefile
	src/Makefile
	include/Makefile
	include/bcg729/Makefile
	test/Makefile
	test/bin/Makefile
	libbcg729.pc
	bcg729.spec
])


AC_OUTPUT
