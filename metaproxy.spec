Summary: Z39.50/SRU router
Name: metaproxy
Version: 1.2.1
Release: 1
License: GPL
Group: Applications/Internet
Vendor: Index Data ApS <info@indexdata.dk>
Source: metaproxy-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-root
BuildRequires: pkgconfig, libyazpp4, libxslt-devel, boost-devel
Packager: Adam Dickmeiss <adam@indexdata.dk>
URL: http://www.indexdata.com/metaproxy
Group:  Applications/Internet
# Requires: 

%description
Metaproxy daemon.

%package doc
Summary: Metaproxy documentation
Group: Documentation

%description doc
Metaproxy documentation.

%package -n libmetaproxy3
Summary: Metaproxy library
Group: Libraries
Requires: libyazpp4

%description -n libmetaproxy3
The Metaproxy libraries.

%package -n libmetaproxy3-devel
Summary: Metaproxy development package
Group: Development/Libraries
Requires: libmetaproxy3 = %{version} libyazpp4-devel

%description -n libmetaproxy3-devel
Development libraries and include files for the Metaproxy package.

%prep
%setup

%build

CFLAGS="$RPM_OPT_FLAGS" \
 ./configure --prefix=%{_prefix} --libdir=%{_libdir} --mandir=%{_mandir} \
	--enable-shared --with-yazpp=/usr/bin
make CFLAGS="$RPM_OPT_FLAGS"

%install
rm -fr ${RPM_BUILD_ROOT}
make prefix=${RPM_BUILD_ROOT}/%{_prefix} mandir=${RPM_BUILD_ROOT}/%{_mandir} \
	libdir=${RPM_BUILD_ROOT}/%{_libdir} install
rm ${RPM_BUILD_ROOT}/%{_libdir}/*.la
rm -fr ${RPM_BUILD_ROOT}/%{_prefix}/share/metaproxy
rm -f ${RPM_BUILD_ROOT}/%{_libdir}/metaproxy/*
mkdir -p ${RPM_BUILD_ROOT}/%{_libdir}/metaproxy/modules
mkdir -p ${RPM_BUILD_ROOT}/etc/metaproxy/filters-enabled
mkdir -p ${RPM_BUILD_ROOT}/etc/metaproxy/filters-available
mkdir -p ${RPM_BUILD_ROOT}/etc/logrotate.d
mkdir -p ${RPM_BUILD_ROOT}/etc/rc.d/init.d
install -m 644 rpm/metaproxy.xml ${RPM_BUILD_ROOT}/etc/metaproxy/metaproxy.xml
install -m 755 rpm/metaproxy.init ${RPM_BUILD_ROOT}/etc/rc.d/init.d/metaproxy
install -m 644 rpm/metaproxy.logrotate  ${RPM_BUILD_ROOT}/etc/logrotate.d/metaproxy

%clean
rm -fr ${RPM_BUILD_ROOT}

%files -n libmetaproxy3
%doc README LICENSE NEWS
%defattr(-,root,root)
%{_libdir}/*.so.*
%dir %{_libdir}/metaproxy/modules

%post -n libmetaproxy3
/sbin/ldconfig

%postun -n libmetaproxy3
/sbin/ldconfig

%files -n libmetaproxy3-devel
%defattr(-,root,root)
%{_includedir}/metaproxy
%{_libdir}/*.so
%{_libdir}/*.a

%files doc
%defattr(-,root,root)
%{_prefix}/share/doc/metaproxy

%files
%defattr(-,root,root)
%{_bindir}/metaproxy
%{_mandir}/man?/*
%config /etc/rc.d/init.d/metaproxy
%config(noreplace) /etc/metaproxy/metaproxy.xml
%dir /etc/metaproxy/filters-available
%dir /etc/metaproxy/filters-enabled
%config(noreplace) /etc/logrotate.d/metaproxy

%post
if [ $1 = 1 ]; then
        /sbin/chkconfig --add metaproxy
        /sbin/service metaproxy start > /dev/null 2>&1
else
        /sbin/service metaproxy restart > /dev/null 2>&1
fi
%preun
if [ $1 = 0 ]; then
        /sbin/service metaproxy stop > /dev/null 2>&1
        /sbin/chkconfig --del metaproxy
fi
