#---------------------------------------------
# spec file for package mustach
#---------------------------------------------
%global site    https://gitlab.com/jobol/mustach

Name:           mustach
#Hexsha: None
Version: 	1.2.2
Release: 	2%{?dist}
License:        ISC
Summary:        Tiny Mustach processor
Url:            %{site}

Source:         %{site}/-/archive/%{version}/%{name}-%{version}.tar.bz2

BuildRequires:  make
BuildRequires:  gcc-c++
BuildRequires:  pkgconfig(json-c)
BuildRequires:  pkgconfig(libcjson)
BuildRequires:  pkgconfig(jansson)

%description
Tiny tool for processing JSON files with Mustache templates.

#---------------------------------------------
%package lib-core

Summary:        Core library of mustach

%description lib-core
Core library of mustach

#---------------------------------------------
%package lib-core-devel

Summary:        Core library of mustach - Development files
Requires:       %{name}-lib-core = %{version}

%description lib-core-devel
Development files for mustach core library


#---------------------------------------------
%package lib-json-c

Summary:        json-c library of mustach
Requires:       pkgconfig(json-c)
Provides:       pkgconfig(libmustach-json-c) = %{version}

%description lib-json-c
Mustach library for using json-c library

#---------------------------------------------
%package lib-json-c-devel

Summary:        json-c library of mustach - Development files
Requires:       %{name}-lib-json-c = %{version}
Provides:       pkgconfig(libmustach-json-c) = %{version}

%description lib-json-c-devel
Development files for mustach library using json-c library


#---------------------------------------------
%package lib-cjson

Summary:        cjson library of mustach
Requires:       pkgconfig(cjson)
Provides:       pkgconfig(libmustach-cjson) = %{version}

%description lib-cjson
Mustach library for using json-c library

#---------------------------------------------
%package lib-cjson-devel

Summary:        cjson library of mustach - Development files
Requires:       %{name}-lib-cjson = %{version}
Provides:       pkgconfig(libmustach-cjson) = %{version}

%description lib-cjson-devel
Development files for mustach library using json-c library


#---------------------------------------------
%package lib-jansson

Summary:        jansson library of mustach
Requires:       pkgconfig(jansson)
Provides:       pkgconfig(libmustach-jansson) = %{version}

%description lib-jansson
Mustach library for using json-c library

#---------------------------------------------
%package lib-jansson-devel

Summary:        jansson library of mustach - Development files
Requires:       %{name}-lib-jansson = %{version}
Provides:       pkgconfig(libmustach-jansson) = %{version}

%description lib-jansson-devel
Development files for mustach library using json-c library


#---------------------------------------------
%global pkgdir %{_libdir}/pkgconfig

%global mustach_conf tool=jsonc libs=split jsonc=yes cjson=yes jansson=yes

%global mustach_env  PREFIX=%{_prefix} DESTDIR=%{buildroot} BINDIR=%{_bindir} LIBDIR=%{_libdir} INCLUDEDIR=%{_includedir} MANDIR=%{_mandir} PKGDIR=%{pkgdir}

%global mustach_flags CFLAGS="-O2 -g"

%global mustach_make  %{mustach_flags} %__make %{?_smp_mflags} %mustach_conf %mustach_env

#---------------------------------------------
%prep
%setup -q -n %{name}-%{version}

%build
%mustach_make

%install
%mustach_make install

#---------------------------------------------
%files
%defattr(-,root,root)
%{_bindir}/mustach
%{_mandir}/man1/mustach.1.gz

#---------------------------------------------
%post lib-core
/sbin/ldconfig

%postun lib-core
/sbin/ldconfig

%files lib-core
%defattr(-,root,root)
%{_libdir}/libmustach-core.so.*

#---------------------------------------------
%files lib-core-devel
%defattr(-,root,root)
%{_libdir}/libmustach-core.so
%{pkgdir}/libmustach-core.pc
%{_includedir}/mustach/mustach.h
%{_includedir}/mustach/mustach-wrap.h

#---------------------------------------------
%post lib-json-c
/sbin/ldconfig

%postun lib-json-c
/sbin/ldconfig

%files lib-json-c
%defattr(-,root,root)
%{_libdir}/libmustach-json-c.so.*

#---------------------------------------------
%files lib-json-c-devel
%defattr(-,root,root)
%{_libdir}/libmustach-json-c.so
%{pkgdir}/libmustach-json-c.pc
%{_includedir}/mustach/mustach-json-c.h

#---------------------------------------------
%post lib-cjson
/sbin/ldconfig

%postun lib-cjson
/sbin/ldconfig

%files lib-cjson
%defattr(-,root,root)
%{_libdir}/libmustach-cjson.so.*

#---------------------------------------------
%files lib-cjson-devel
%defattr(-,root,root)
%{_libdir}/libmustach-cjson.so
%{pkgdir}/libmustach-cjson.pc
%{_includedir}/mustach/mustach-cjson.h

#---------------------------------------------
%post lib-jansson
/sbin/ldconfig

%postun lib-jansson
/sbin/ldconfig

%files lib-jansson
%defattr(-,root,root)
%{_libdir}/libmustach-jansson.so.*

#---------------------------------------------
%files lib-jansson-devel
%defattr(-,root,root)
%{_libdir}/libmustach-jansson.so
%{pkgdir}/libmustach-jansson.pc
%{_includedir}/mustach/mustach-jansson.h

#---------------------------------------------
%changelog

* Thu Oct 28 2021 José Bollo jose.bollo@iot.bzh 1.2.2
- Initial version
