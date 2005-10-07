## -*- Autoconf -*-
## Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT(yp2, 0.0.1, marc@indexdata.dk)
AM_CONFIG_HEADER(src/config.hpp)


AC_CONFIG_SRCDIR(configure.ac) 
AC_CONFIG_AUX_DIR(config)
AC_CANONICAL_SYSTEM
##AC_PREFIX_DEFAULT(`pwd`/install) 

AM_INIT_AUTOMAKE


# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AM_PROG_LIBTOOL

# Use C++ compilers only for tests
AC_LANG(C++)

# Checks for libraries.
##AC_CHECK_LIB(ibrary, main)
AC_CHECK_LIB(boost_thread, main, [],[
	AC_MSG_ERROR([boost thread library not found. Install libboost-thread-dev (or similar)])
])

## AC_CHECK_LIB(boost_unit_test_framework, [], [],[
## 	AC_MSG_ERROR([boost unit test library not found. Install libboost-test-dev (or similar)])
## ])

## YAZPP checks
YAZPP_INIT(threads,1.0)
if test -z "$YAZPPLIB"; then
	AC_MSG_ERROR([YAZ++ development libraries missing])
fi
##YAZ_DOC

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
                if test "$xsltdir" = "default"; then
                        AC_MSG_WARN([Libxslt development libraries not found.])
                else
                        AC_MSG_ERROR([libxslt development libraries not found.])                fi
	fi
fi


# Checks for header files.
##AC_HEADER_STDC
##AC_HEADER_STDCXX
##AC_HEADER_SYS_WAIT
##AC_HEADER_TIME 
AC_CHECK_HEADERS(boost/thread/mutex.hpp)



# Checks for typedefs, structures, and compiler characteristics.
##AC_TYPE_SIZE_T 
##AC_CHECK_SIZEOF(int)
##AC_CHECK_SIZEOF(long int)


# Checks for library functions.

AC_CONFIG_FILES([
        Doxyfile
	Makefile
	src/Makefile
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

------------------------------------------------------------------------"
