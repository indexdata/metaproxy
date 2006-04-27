## -*- Autoconf -*-
## Process this file with autoconf to produce a configure script.

# Autoconf and automake setup
AC_PREREQ(2.59)
AC_INIT([metaproxy],[1.0.1], [adam@indexdata.dk])

AC_CONFIG_HEADERS(src/config.hpp)

AC_CONFIG_SRCDIR([configure.ac]) 
AC_CONFIG_AUX_DIR([config])
AC_CANONICAL_SYSTEM
##AC_PREFIX_DEFAULT(`pwd`/install) 

AM_INIT_AUTOMAKE([1.8])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AM_PROG_LIBTOOL

# Use C++ compilers only for tests
AC_LANG(C++)

if test "$enable_shared" = "yes"; then
   	AC_DEFINE([HAVE_DL_SUPPORT],[1],
                [Define to 1 if you have shared module support])
fi

# Checks for libraries.
AC_CHECK_LIB(dl,dlopen)

AC_CHECK_LIB(boost_thread, main, [],[
	AC_MSG_NOTICE([boost thread library not found.])
	AC_MSG_ERROR([Install libboost-thread-dev (or similar)])
])

AC_CHECK_LIB(boost_date_time, main, [],[
	AC_MSG_NOTICE([boost date_time library not found.])
	AC_MSG_ERROR([Install libboost-date-time-dev (or similar)])
])

AC_CHECK_LIB(boost_program_options, main, [],[
	AC_MSG_NOTICE([boost program options library not found.])
	AC_MSG_ERROR([Install libboost-program-options-dev (or similar)])
])

AC_CHECK_HEADER(boost/test/auto_unit_test.hpp,,[
	AC_MSG_NOTICE([boost test unit header not found.])
	AC_MSG_ERROR([Install libboost-test-dev (or similar)])
])

AC_MSG_CHECKING([whether boost_unit_test_framework works])
OLIBS=$LIBS
LIBS="$LIBS -lboost_unit_test_framework"
AC_TRY_LINK([
#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp> 
BOOST_AUTO_UNIT_TEST( test )
{
 BOOST_CHECK(true);
}
],[],
[
	AC_MSG_RESULT([yes])
	LIBS=$OLIBS
],
[
	AC_MSG_RESULT([no])
	AC_MSG_NOTICE([Boost unit test framework does not appear to work])
	AC_MSG_ERROR([Consult config.log for details])
])

## YAZPP checks
YAZPP_INIT(threads,1.0)
if test -z "$YAZPPLIB"; then
	AC_MSG_ERROR([YAZ++ development libraries missing])
fi
YAZ_DOC

## libxslt checks
AC_SUBST(XSLT_LIBS)
AC_SUBST(XSLT_CFLAGS)
xsltdir=default
AC_ARG_WITH(xslt,[[  --with-xslt[=PREFIX]    use libxslt in PREFIX]],xsltdir=$withval)
if test "$xsltdir" = "yes" -o "$xsltdir" = "default"; then
	for d in /usr /usr/local; do
		if test -x $d/bin/xslt-config; then
			xsltdir=$d
		fi
	done
fi
if test "$xsltdir" != "no"; then
	AC_MSG_CHECKING(for libXSLT)
	if test -x $xsltdir/bin/xslt-config; then
		XSLT_LIBS=`$xsltdir/bin/xslt-config --libs`
		XSLT_CFLAGS=`$xsltdir/bin/xslt-config --cflags`
		XSLT_VER=`$xsltdir/bin/xslt-config --version`
		AC_MSG_RESULT($XSLT_VER)
		AC_DEFINE([HAVE_XSLT],[1],
                [Define to 1 if you have libxslt installed])
	else
		AC_MSG_RESULT(Not found)
		AC_MSG_ERROR([libxslt required.])
	fi
fi


# Checks for header files.
##AC_HEADER_STDC
##AC_HEADER_SYS_WAIT
##AC_HEADER_TIME 
AC_CHECK_HEADERS(sys/select.h sys/socket.h iostream list vector stdexcept)
AC_CHECK_HEADERS(boost/thread/mutex.hpp \ 
                 boost/date_time/posix_time/posix_time.hpp \
                 boost/regex.hpp \
                 boost/test/auto_unit_test.hpp)

# Checks for typedefs, structures, and compiler characteristics.
##AC_TYPE_SIZE_T 
##AC_CHECK_SIZEOF(int)
##AC_CHECK_SIZEOF(long int)


# Checks for library functions.

AC_CONFIG_FILES([
        Doxyfile
	Makefile
	src/Makefile
	src/Jamfile
	doc/Makefile
	doc/main.xml
        doc/common/Makefile
        doc/common/html.dsl
        doc/common/html.xsl
        doc/common/print.dsl
        doc/common/tkl.xsl
	etc/Makefile
])

AC_OUTPUT


# postamble

echo \
"------------------------------------------------------------------------
Configuration:

  Source code location:       ${srcdir}
  CPP Preprocessor flags:     ${CPPFLAGS}
  CXX Compiler:               ${CXX}
  CXX Compiler flags:         ${CXXFLAGS}
  Linker flags:               ${LDFLAGS}
  Linked libs:                ${LIBS}
  Host System Type:           ${host}
  Install path:               ${prefix}
  Automake:                   ${AUTOMAKE}

------------------------------------------------------------------------"
