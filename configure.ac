## -*- Autoconf -*-
## Process this file with autoconf to produce a configure script.

# Autoconf and automake setup
AC_PREREQ([2.69])
AC_INIT([metaproxy],[m4_esyscmd([. ./IDMETA; printf $VERSION])],[info@indexdata.com])

AC_CONFIG_HEADERS([src/config.hpp])

AC_CONFIG_SRCDIR([configure.ac]) 
AC_CONFIG_AUX_DIR([config])
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([1.9])

AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
LT_INIT

AC_CHECK_PROGS([TRANG], [trang], [echo])

# Default language to use for tests (some tests may switch to C++)
AC_LANG([C])

if test "$enable_shared" = "yes"; then
    AC_DEFINE([HAVE_DL_SUPPORT],[1],
	[Define to 1 if you have shared module support])
fi

# Checks for libraries.
AC_CHECK_LIB([dl],[dlopen])
AC_CHECK_LIB([m],[main])

AC_CHECK_FUNCS([localtime_r getrlimit])

USEMARCON_INIT

## YAZPP checks
YAZPP_INIT([threads],[1.9.0])
if test -z "$YAZPPLIB"; then
    AC_MSG_ERROR([YAZ++ development libraries missing])
fi
LIBS="$LIBS -lpthread"
CPPFLAGS="$YAZPPINC $CPPFLAGS"
AC_MSG_CHECKING([if YAZ is version 5.35.0 or later])
AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM([[#include <yaz/yaz-version.h>]],
                         [[
#if YAZ_VERSIONL < 0x52300
#error too old
#endif
]])],
        [AC_MSG_RESULT([yes])],
        [AC_MSG_RESULT([no])
	 AC_MSG_ERROR([newer version of YAZ required])]
)
YAZ_DOC

ID_BOOST([system thread test regex],[1.34])
if test -z "${BOOST_THREAD_LIB}"; then
    AC_MSG_ERROR([Boost thread development libraries required])
fi
if test -z "${BOOST_TEST_LIB}"; then
    AC_MSG_ERROR([Boost unit test framework libraries required])
fi
if test -z "${BOOST_REGEX_LIB}"; then
    AC_MSG_ERROR([Boost regex development libraries required])
fi

case $YAZPPINC in
  *YAZ_HAVE_XSLT=1*) ;;
  *) AC_MSG_ERROR([YAZ not compiled with Libxslt support]) ;;
esac

AC_CHECK_HEADERS([sys/select.h sys/socket.h])

if test -d ${srcdir}/.git; then
    SHA1=`git show --pretty=format:%H|head -1`
else
    SHA1=`head -1 ${srcdir}/ChangeLog|awk '{print $2}'`
fi
AC_DEFINE_UNQUOTED([VERSION_SHA1], "$SHA1", [SHA-1 from Git])

AC_CONFIG_FILES([
	metaproxy-config
        Doxyfile
	Makefile
	include/Makefile include/metaproxy/Makefile
	src/Makefile
	xml/Makefile
	xml/schema/Makefile
	xml/xslt/Makefile
	doc/Makefile
	doc/local0.ent
	doc/common/Makefile
	doc/common/print.dsl
	etc/Makefile
	metaproxy.pc
])
AC_CONFIG_COMMANDS([default],[
	sed -e 's%echo_source=yes%echo_source=no%g; s%src_root=.*$%%g; s%build_root=.*%%g' \
		< metaproxy-config > src/metaproxy-config && chmod +x metaproxy-config src/metaproxy-config
	diff doc/local.ent doc/local0.ent >/dev/null 2>/dev/null \
		|| cp doc/local0.ent doc/local.ent
],[])
AC_OUTPUT

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
  Boost Libs:                 ${BOOST_DATE_TIME_LIB} ${BOOST_THREAD_LIB}               
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
