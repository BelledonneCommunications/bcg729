# -*- rpm-spec -*-

%define _prefix    @CMAKE_INSTALL_PREFIX@
%define pkg_prefix @BC_PACKAGE_NAME_PREFIX@

# re-define some directories for older RPMBuild versions which don't. This messes up the doc/ dir
# taken from https://fedoraproject.org/wiki/Packaging:RPMMacros?rd=Packaging/RPMMacros
%define _datarootdir       %{_prefix}/share
%define _datadir           %{_datarootdir}
%define _docdir            %{_datadir}/doc

%define build_number 1



Name:           @CPACK_PACKAGE_NAME@
Version:        @PROJECT_VERSION@
Release:        %{build_number}%{?dist}
Summary:        Bcg729 is an opensource implementation of both encoder and decoder of the ITU G729 Annex A/B speech codec.

Group:          Applications/Communications
License:        GPL
URL:            http://www.linphone.org
Source0:        %{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-buildroot

Requires:	%{pkg_prefix}bctoolbox

%description

Bcg729 is an opensource implementation of both encoder and decoder of the ITU G729 Annex A/B speech codec.

The library written in C 99 is fully portable and can be executed on many platforms including both ARM and x86 processors.
libbcg729 supports concurrent channels encoding/decoding for multi call application such as conferencing.
This project was initially developed as part of Mediastreamer2, the Linphone's media processing engine.
This is why it also contains the glue to be integrated in Linphone/Mediastreamer2.


%package devel
Summary:       Development libraries for bcg729
Group:         Development/Libraries
Requires:      %{name} = %{version}-%{release}

%description    devel
Libraries and headers required to develop software with bcg729

%if 0%{?rhel} && 0%{?rhel} <= 7
%global cmake_name cmake3
%define ctest_name ctest3
%else
%global cmake_name cmake
%define ctest_name ctest
%endif

# This is for debian builds where debug_package has to be manually specified, whereas in centos it does not
%define custom_debug_package %{!?_enable_debug_packages:%debug_package}%{?_enable_debug_package:%{nil}}
%custom_debug_package

%prep
%setup -n %{name}-%{version}

%build
%{expand:%%%cmake_name} . -DCMAKE_BUILD_TYPE=@CMAKE_BUILD_TYPE@ -DCMAKE_PREFIX_PATH:PATH=%{_prefix}
make %{?_smp_mflags}

%install
make install DESTDIR=%{buildroot}

# Dirty workaround to give exec rights for all shared libraries. Debian packaging needs this
# TODO : set CMAKE_INSTALL_SO_NO_EXE for a cleaner workaround
chmod +x `find %{buildroot} *.so.*`


%check
%{ctest_name} -V %{?_smp_mflags}

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%defattr(-,root,root)
%doc AUTHORS.md CHANGELOG.md LICENSE.txt README.md
%{_libdir}/*.so.*

%files devel
%defattr(-,root,root)
%{_includedir}/bcg729
%{_libdir}/libbcg729.a
%{_libdir}/libbcg729.so
%{_datadir}/Bcg729/cmake/Bcg729Config*.cmake
%{_datadir}/Bcg729/cmake/Bcg729Targets*.cmake

%changelog

* Tue Nov 27 2018 ronan.abhamon <ronan.abhamon@belledonne-communications.com>
- Do not set CMAKE_INSTALL_LIBDIR and never with _libdir!
