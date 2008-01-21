## -*- Autoconf -*-
## Process this file with autoconf to produce a configure script.

# Autoconf and automake setup
AC_PREREQ(2.59)
AC_INIT([metaproxy],[1.0.13],[metaproxy-help@indexdata.dk])

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

# Default language to use for tests (some tests may switch to C++)
AC_LANG(C)

if test "$enable_shared" = "yes"; then
    AC_DEFINE([HAVE_DL_SUPPORT],[1],
	[Define to 1 if you have shared module support])
fi

# Checks for libraries.
AC_CHECK_LIB(dl,dlopen)
AC_CHECK_LIB(m,main)

## YAZPP checks
YAZPP_INIT([threads],[1.1.0])
if test -z "$YAZPPLIB"; then
    AC_MSG_ERROR([YAZ++ development libraries missing])
fi
YAZ_DOC
CPPFLAGS="$YAZPPINC $CPPFLAGS"

# check for Boost C++ development libs and headers

AX_BOOST([1.32])
if test "$ax_cv_boost_base" = "no"; then
    AC_MSG_ERROR([install libboost-dev or similar])
fi
if test "$ax_cv_boost_date_time" = "no"; then
    AC_MSG_ERROR([install libboost-date-time-dev or similar])
fi
if test "$ax_cv_boost_program_options" = "no"; then
    AC_MSG_ERROR([install libboost-program-options-dev or similar])
fi
if test "$ax_cv_boost_thread" = "no"; then
    AC_MSG_ERROR([install libboost-thread-dev or similar])
fi
if test "$ax_cv_boost_unit_test_framework" = "no"; then
    AC_MSG_ERROR([install libboost-test-dev or similar])
fi
    


# Checks for header files.
##AC_HEADER_STDC
##AC_HEADER_SYS_WAIT
##AC_HEADER_TIME 
AC_CHECK_HEADERS(sys/select.h sys/socket.h)

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
	xml/Makefile
	xml/schema/Makefile
	xml/xslt/Makefile
	doc/Makefile
	doc/local.ent
        doc/common/Makefile
	doc/common/print.dsl
	etc/Makefile
])

AC_OUTPUT


# postamble


echo \
"------------------------------------------------------------------------

  Package:                    ${PACKAGE}
  Version:                    ${VERSION}
  Source code location:       ${srcdir}
  CPP Preprocessor:           ${CPP}
  CPP Preprocessor flags:     ${CPPFLAGS}
  CXX Compiler:               ${CXX}
  CXX Compiler flags:         ${CXXFLAGS}
  Linker flags:               ${LDFLAGS}
  Linked libs:                ${LIBS}
  Host System Type:           ${host}
  Install path:               ${prefix}
  Automake:                   ${AUTOMAKE}
  Archiver:                   ${AR}
  Ranlib:                     ${RANLIB}
  Boost Version:              ${BOOST_VERSION}               
  Boost Include:              ${BOOST_CPPFLAGS}               
  Boost linker flags:         ${BOOST_LDFLAGS}               
  Boost Libs:                 ${BOOST_DATE_TIME_LIB} ${BOOST_PROGRAM_OPTIONS_LIB} ${BOOST_THREAD_LIB}               
  YAZPP Version:              ${YAZPPVERSION}
  YAZPP Include:              ${YAZPPINC}
  YAZPP La Lib:               ${YAZPPLALIB}
  YAZPP Lib:                  ${YAZPPLIB}
  Bugreport:                  ${PACKAGE_BUGREPORT}

------------------------------------------------------------------------"
dnl Local Variables:
dnl mode:shell-script
dnl sh-indentation: 2
dnl sh-basic-offset: 4
dnl End:
