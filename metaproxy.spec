%define idmetaversion %(. ./IDMETA; echo $VERSION)
Summary: Z39.50/SRU router
Name: metaproxy
Version: %{idmetaversion}
Release: 1.indexdata
License: GPL
Group: Applications/Internet
Vendor: Index Data ApS <info@indexdata.dk>
Source: metaproxy-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-root
Prefix: %{_prefix} /etc/metaproxy
BuildRequires: pkgconfig, libyaz5-devel >= 5.35.0, libyazpp7-devel >= 1.9.0
BuildRequires: libxslt-devel, boost-devel
Conflicts: cf-engine <= 2.12.5
Packager: Adam Dickmeiss <adam@indexdata.dk>
URL: http://www.indexdata.com/metaproxy

# Use systemd macros for safe scriptlets
%{?systemd_requires}

Requires:  libmetaproxy6 = %{version}
Provides: metaproxy6

%description
Metaproxy daemon.

%package doc
Summary: Metaproxy documentation
Group: Documentation

%description doc
Metaproxy documentation.

%package -n libmetaproxy6
Summary: Metaproxy library
Group: Libraries
Requires: libyazpp7 >= 1.8.0, libyaz5 >= 5.30.0

%description -n libmetaproxy6
The Metaproxy libraries.

%package -n libmetaproxy6-devel
Summary: Metaproxy development package
Group: Development/Libraries
Requires: libmetaproxy6 = %{version}, libyazpp7-devel, boost-devel
Conflicts: libmetaproxy3-devel, libmetaproxy4-devel, libmetaproxy5-devel

%description -n libmetaproxy6-devel
Development libraries and include files for the Metaproxy package.

%prep
%setup

%build
CFLAGS="$RPM_OPT_FLAGS" \
 ./configure --prefix=%{_prefix} --libdir=%{_libdir} --mandir=%{_mandir} \
	--enable-shared --with-yazpp=pkg
%if %{?make_build:1}%{!?make_build:0}
%make_build
%else
make -j4 CFLAGS="$RPM_OPT_FLAGS"
%endif

%install
rm -fr ${RPM_BUILD_ROOT}
make install DESTDIR=${RPM_BUILD_ROOT}
rm ${RPM_BUILD_ROOT}/%{_libdir}/*.la
rm -f ${RPM_BUILD_ROOT}/%{_libdir}/metaproxy/*
mkdir -p ${RPM_BUILD_ROOT}/%{_libdir}/metaproxy6/modules
mkdir -p ${RPM_BUILD_ROOT}/etc/metaproxy/filters-enabled
mkdir -p ${RPM_BUILD_ROOT}/etc/metaproxy/filters-available
mkdir -p ${RPM_BUILD_ROOT}/etc/metaproxy/ports.d
mkdir -p ${RPM_BUILD_ROOT}/etc/metaproxy/routes.d
mkdir -p ${RPM_BUILD_ROOT}/etc/logrotate.d
mkdir -p ${RPM_BUILD_ROOT}/etc/init.d
mkdir -p ${RPM_BUILD_ROOT}/etc/sysconfig
mkdir -p ${RPM_BUILD_ROOT}/etc/systemd/system
install -m 644 rpm/metaproxy.xml ${RPM_BUILD_ROOT}/etc/metaproxy/metaproxy.xml
install -m 644 rpm/metaproxy.user ${RPM_BUILD_ROOT}/etc/metaproxy/metaproxy.user
install -m 644 rpm/metaproxy.service ${RPM_BUILD_ROOT}/etc/systemd/system/metaproxy.service
install -m 644 rpm/metaproxy.sysconfig ${RPM_BUILD_ROOT}/etc/sysconfig/metaproxy
install -m 644 rpm/metaproxy.logrotate  ${RPM_BUILD_ROOT}/etc/logrotate.d/metaproxy

%clean
rm -fr ${RPM_BUILD_ROOT}

%files -n libmetaproxy6
%doc README.md LICENSE NEWS
%defattr(-,root,root)
%{_libdir}/*.so.*
%dir %{_libdir}/metaproxy6/modules

%post -n libmetaproxy6 -p /sbin/ldconfig
%postun -n libmetaproxy6 -p /sbin/ldconfig

%files -n libmetaproxy6-devel
%defattr(-,root,root)
%{_includedir}/metaproxy
%{_libdir}/pkgconfig/*.pc
%{_libdir}/*.so
%{_libdir}/*.a
%{_bindir}/metaproxy-config
%{_mandir}/man1/metaproxy-config.*

%files doc
%defattr(-,root,root)
%{_prefix}/share/doc/metaproxy

%files
%defattr(-,root,root)
%{_datadir}/metaproxy
%{_bindir}/metaproxy
%{_mandir}/man3/*
%{_mandir}/man1/metaproxy.*
%config /etc/systemd/system/metaproxy.service
%config(noreplace) /etc/metaproxy/metaproxy.xml
%config /etc/metaproxy/metaproxy.user
%dir /etc/metaproxy/filters-available
%dir /etc/metaproxy/filters-enabled
%dir /etc/metaproxy/ports.d
%dir /etc/metaproxy/routes.d
%config(noreplace) /etc/logrotate.d/metaproxy
%config(noreplace) /etc/sysconfig/metaproxy

%post
. /etc/metaproxy/metaproxy.user 2>/dev/null || :

# Ensure group exists
if [ -n "$SERVER_GROUP" ] && ! getent group | grep -q "^$SERVER_GROUP:" ; then
    groupadd -r "$SERVER_GROUP" 2>/dev/null || :
fi

# Ensure user exists
if [ -n "$SERVER_USER" ] && ! getent passwd | grep -q "^$SERVER_USER:" ; then
    useradd -r -s /sbin/nologin -c "${SERVER_NAME:-Metaproxy}" \
        -d "${SERVER_HOME:-/var/lib/metaproxy}" \
        -g "${SERVER_GROUP:-metaproxy}" \
        "$SERVER_USER" 2>/dev/null || :
fi

# Ensure home directory exists
if [ -n "$SERVER_HOME" ] && [ ! -d "$SERVER_HOME" ]; then
    mkdir -p "$SERVER_HOME"
    chown "$SERVER_USER:$SERVER_GROUP" "$SERVER_HOME" 2>/dev/null || :
fi

# Safe systemd handling (won't fail in containers)
%systemd_post metaproxy.service

%preun
%systemd_preun metaproxy.service

if [ "$1" = 0 ]; then
    . /etc/metaproxy/metaproxy.user 2>/dev/null || :
    if [ -n "$SERVER_HOME" ] && [ -d "$SERVER_HOME" ]; then
        rm -rf "$SERVER_HOME" || :
    fi
    if [ -n "$SERVER_USER" ]; then
        userdel "$SERVER_USER" 2>/dev/null || :
    fi
fi

%postun
%systemd_postun_with_restart metaproxy.service
