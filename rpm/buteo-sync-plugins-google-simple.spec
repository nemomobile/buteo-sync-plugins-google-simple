Name: buteo-sync-plugins-google-simple
Version: 0.0.2
Release: 1
Summary: One-way Buteo Google Contacts synchronization plugin
Group: System/Libraries
URL: https://github.com/nemomobile/buteo-sync-plugins-google-simple
License: LGPLv2.1
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5Contacts)
BuildRequires: pkgconfig(Qt5Network)
BuildRequires: pkgconfig(Qt5Test)
BuildRequires: pkgconfig(buteosyncfw5)
BuildRequires: pkgconfig(libsignon-qt5)
BuildRequires: pkgconfig(accounts-qt5)
BuildRequires: pkgconfig(libsailfishkeyprovider)
BuildRequires: qtcontacts-sqlite-qt5

%description
%{summary}.

%files
%defattr(-,root,root,-)
%config %{_sysconfdir}/buteo/profiles/client/googlecontactssimple.xml
%config %{_sysconfdir}/buteo/profiles/sync/google.Contacts.xml
%{_libdir}/buteo-plugins-qt5/*.so

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5 -recursive
make

%install
make INSTALL_ROOT=%{buildroot} install
