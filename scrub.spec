Name: scrub
Version: 1.8
Release: 1
Summary: Disk scrubbing program
License: GPL
Group: System Environment/Base
Url: http://sourceforge.net/projects/diskscrub/
Source0: scrub-%{version}.tar.bz2
BuildRoot: %{_tmppath}/%{name}-%{version}-root-%(%{__id_u} -n)

%description
This utility writes patterns on files or disk devices to make
retrieving the data more difficult.  It operates in one of three
modes: 1) the special file corresponding to an entire disk is scrubbed 
and all data on it is destroyed;  2) a regular file is scrubbed and 
only the data in the file (and optionally its name in the directory 
entry) is destroyed; or 3) a regular file is created, expanded until 
the file system is full, then scrubbed as in 2). 

%prep
%setup

%build
make

%install
mkdir -p $RPM_BUILD_ROOT%{_bindir}
mkdir -p $RPM_BUILD_ROOT%{_mandir}/man1
install -s -m 755 scrub $RPM_BUILD_ROOT/%{_bindir}
install -m 644 scrub.1 $RPM_BUILD_ROOT/%{_mandir}/man1

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc README ChangeLog DISCLAIMER COPYING
%{_bindir}/scrub
%{_mandir}/man*/*

%changelog
* Tue Feb 14 2006 Ben Woodard <woodard@redhat.com> 1.7-1
- Add license file to documents
- Add full path to source
- Add some braces

* Thu Feb 9 2006 Ben Woodard <woodard@redhat.com> 1.6-4
- changed build root to match fedora guidelines.

* Wed Feb 1 2006 Ben Woodard <woodard@redhat.com> 1.6-2
- fixed some problems uncovered by rpmlint
