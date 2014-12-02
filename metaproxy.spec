%define idmetaversion %(. ./IDMETA; echo $VERSION|tr -d '\n')
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
BuildRequires: pkgconfig, libyaz5-devel >= 5.7.0, libyazpp6-devel >= 1.6.0
BuildRequires: libxslt-devel, boost-devel
Conflicts: cf-engine <= 2.12.5
Packager: Adam Dickmeiss <adam@indexdata.dk>
URL: http://www.indexdata.com/metaproxy
Group:  Applications/Internet
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
Requires: libyazpp6 >= 1.6.0, libyaz5 >= 5.7.0

%description -n libmetaproxy6
The Metaproxy libraries.

%package -n libmetaproxy6-devel
Summary: Metaproxy development package
Group: Development/Libraries
Requires: libmetaproxy6 = %{version}, libyazpp6-devel, boost-devel
Conflicts: libmetaproxy3-devel, libmetaproxy4-devel, libmetaproxy5-devel

%description -n libmetaproxy6-devel
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
make install DESTDIR=${RPM_BUILD_ROOT}
rm ${RPM_BUILD_ROOT}/%{_libdir}/*.la
rm -fr ${RPM_BUILD_ROOT}/%{_prefix}/share/metaproxy
rm -f ${RPM_BUILD_ROOT}/%{_libdir}/metaproxy/*
mkdir -p ${RPM_BUILD_ROOT}/%{_libdir}/metaproxy6/modules
mkdir -p ${RPM_BUILD_ROOT}/etc/metaproxy/filters-enabled
mkdir -p ${RPM_BUILD_ROOT}/etc/metaproxy/filters-available
mkdir -p ${RPM_BUILD_ROOT}/etc/metaproxy/ports.d
mkdir -p ${RPM_BUILD_ROOT}/etc/metaproxy/routes.d
mkdir -p ${RPM_BUILD_ROOT}/etc/logrotate.d
mkdir -p ${RPM_BUILD_ROOT}/etc/init.d
mkdir -p ${RPM_BUILD_ROOT}/etc/sysconfig
install -m 644 rpm/metaproxy.xml ${RPM_BUILD_ROOT}/etc/metaproxy/metaproxy.xml
install -m 644 rpm/metaproxy.user ${RPM_BUILD_ROOT}/etc/metaproxy/metaproxy.user
install -m 755 rpm/metaproxy.init ${RPM_BUILD_ROOT}/etc/init.d/metaproxy
install -m 644 rpm/metaproxy.sysconfig ${RPM_BUILD_ROOT}/etc/sysconfig/metaproxy
install -m 644 rpm/metaproxy.logrotate  ${RPM_BUILD_ROOT}/etc/logrotate.d/metaproxy

%clean
rm -fr ${RPM_BUILD_ROOT}

%files -n libmetaproxy6
%doc README LICENSE NEWS
%defattr(-,root,root)
%{_libdir}/*.so.*
%dir %{_libdir}/metaproxy6/modules

%post -n libmetaproxy6 -p /sbin/ldconfig

%postun -n libmetaproxy6 -p /sbin/ldconfig

%files -n libmetaproxy6-devel
%defattr(-,root,root)
%{_includedir}/metaproxy
%{_libdir}/*.so
%{_libdir}/*.a
%{_bindir}/metaproxy-config
%{_mandir}/man1/metaproxy-config.*

%files doc
%defattr(-,root,root)
%{_prefix}/share/doc/metaproxy

%files
%defattr(-,root,root)
%{_bindir}/metaproxy
%{_mandir}/man3/*
%{_mandir}/man1/metaproxy.*
%config /etc/init.d/metaproxy
%config(noreplace) /etc/metaproxy/metaproxy.xml
%config /etc/metaproxy/metaproxy.user
%dir /etc/metaproxy/filters-available
%dir /etc/metaproxy/filters-enabled
%dir /etc/metaproxy/ports.d
%dir /etc/metaproxy/routes.d
%config(noreplace) /etc/logrotate.d/metaproxy
%config(noreplace) /etc/sysconfig/metaproxy

%post
. /etc/metaproxy/metaproxy.user

 # 1. create group if not existing
if ! getent group | grep -q "^$SERVER_GROUP:" ; then
        echo -n "Adding group $SERVER_GROUP.."
        groupadd -r $SERVER_GROUP 2>/dev/null ||true
        echo "..done"
fi
# 2. create user if not existing
if ! getent passwd | grep -q "^$SERVER_USER:"; then
        echo -n "Adding system user $SERVER_USER.."
        useradd \
            -r \
	    -s /sbin/nologin \
            -c "$SERVER_NAME" \
	    -d $SERVER_HOME \
            -g $SERVER_GROUP \
            $SERVER_USER 2>/dev/null || true
        echo "..done"
fi

if test ! -d $SERVER_HOME; then
	mkdir $SERVER_HOME
	chown $SERVER_USER:$SERVER_GROUP $SERVER_HOME
fi

if [ $1 = 1 ]; then
        /sbin/chkconfig --add metaproxy
        /sbin/service metaproxy start > /dev/null 2>&1
else
        /sbin/service metaproxy restart > /dev/null 2>&1
fi
%preun
if [ $1 = 0 ]; then
	if test -f /etc/init.d/metaproxy; then
        	/sbin/service metaproxy stop > /dev/null 2>&1
        	/sbin/chkconfig --del metaproxy
	fi
	. /etc/metaproxy/metaproxy.user
	test -d $SERVER_HOME && rm -fr $SERVER_HOME
	userdel $SERVER_USER
fi
