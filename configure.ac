AC_INIT([dist_lock], [1.1], [jcm@dim.uchile.cl], [dist_lock], [jcm@dim.uchile.cl])
AC_PREREQ([2.59])
AC_CONFIG_SRCDIR([test/main.cc])
AM_INIT_AUTOMAKE([1.10 -Wall no-define foreign])
AC_CONFIG_HEADERS([config.h])
CFLAGS="${CFLAGS=}"
AM_PROG_AR
AC_ENABLE_STATIC
LT_INIT

AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CXX
AC_LANG([C++])
AX_CXX_COMPILE_STDCXX_11()

AC_HEADER_STDC
AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)
AC_CHECK_LIB(pthread, pthread_create, [PTHREAD_LIBS+=-lpthread] , AC_MSG_ERROR(pthreads is required))
AC_CHECK_LIB(rt, clock_gettime, [CLOCK_LIB+=-lrt] , AC_MSG_ERROR(librt is required))
AC_CHECK_HEADERS(getopt.h)

AC_SEARCH_LIBS([clock_gettime],[rt posix4])

AC_SUBST([CLOCK_LIB])
AC_SUBST([PTHREAD_LIBS])

AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)
AC_LTDL_DLLIB
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
