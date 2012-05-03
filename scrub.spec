Name: scrub
Version: 2.5.0
Release: 1

Summary: Disk scrubbing program
License: GPL
Group: System Environment/Base
Url: https://code.google.com/p/diskscrub/
Source0: scrub-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-root-%(%{__id_u} -n)

%description
Scrub writes patterns on files or disk devices to make
retrieving the data more difficult.  It operates in one of three modes: 
1) the special file corresponding to an entire disk is scrubbed 
   and all data on it is destroyed.
2) a regular file is scrubbed and only the data in the file 
   (and optionally its name in the directory entry) is destroyed.
3) a regular file is created, expanded until 
   the file system is full, then scrubbed as in 2).

%prep
%setup

%build
%configure --program-prefix=%{?_program_prefix:%{_program_prefix}}
make 

%check
make check

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc ChangeLog NEWS DISCLAIMER COPYING INSTALL README
%{_bindir}/scrub
%{_mandir}/man1/*

%changelog
* Tue Feb 14 2006 Ben Woodard <woodard@redhat.com> 1.7-1
- Add license file to documents
- Add full path to source
- Add some braces

* Thu Feb 9 2006 Ben Woodard <woodard@redhat.com> 1.6-4
- changed build root to match fedora guidelines.

* Wed Feb 1 2006 Ben Woodard <woodard@redhat.com> 1.6-2
- fixed some problems uncovered by rpmlint
