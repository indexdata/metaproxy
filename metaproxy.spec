Summary: Z39.50/SRU router
Name: metaproxy
Version: 1.1.4
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
Requires: libmetaproxy3

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
rm -fr ${RPM_BUILD_ROOT}/%{_libdir}/metaproxy
mkdir -p ${RPM_BUILD_ROOT}/etc/metaproxy/filters-enabled
mkdir -p ${RPM_BUILD_ROOT}/etc/metaproxy/filters-available
mkdir -p ${RPM_BUILD_ROOT}/etc/logrotate.d
install -m 644 rpm/metaproxy.xml ${RPM_BUILD_ROOT}/etc/metaproxy/metaproxy.xml
install -m 755 rpm/metaproxy.init ${RPM_BUILD_ROOT}/etc/rc.2/init.d/metaproxy

%clean
rm -fr ${RPM_BUILD_ROOT}

%files -n libmetaproxy3
%doc README LICENSE NEWS
%defattr(-,root,root)
%{_libdir}/*.so.*
%dir %{_libdir}/metaproxy/modules

%files -n libmetaproxy3-devel
%defattr(-,root,root)
%{_includedir}/metaproxy
%{_libdir}/*.so
%{_libdir}/*.a

%files
%defattr(-,root,root)
%{_bindir}/metaproxy
%{_mandir}/man?/*

%files doc
%defattr(-,root,root)
%{_prefix}/share/doc/metaproxy
%config /etc/rc.d/init.d/metaproxy
%config(noreplace) /etc/metaproxy/metaproxy.xml
%dir /etc/metaproxy/filters-available
%dir /etc/metaproxy/filters-enabled
