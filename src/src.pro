#-------------------------------------------------
#
# Buteo Google contacts synchronization plugin
#
#-------------------------------------------------

QT += contacts network
QT -= gui

TARGET = googlecontacts-client
TEMPLATE = lib

CONFIG += link_pkgconfig plugin debug console
PKGCONFIG += buteosyncfw5 libsignon-qt5 accounts-qt5 libsailfishkeyprovider

VER_MAJ = 1
VER_MIN = 0
VER_PAT = 0

QMAKE_CXXFLAGS = -Wall \
    -g \
    -Wno-cast-align \
    -O2 -finline-functions

DEFINES += BUTEOGCONTACTPLUGIN_LIBRARY

SOURCES += \
    googlecontacts.cpp \
    googlestream.cpp \
    googleatom.cpp

HEADERS += \
    googlecontacts.h \
    googlestream.h \
    googleatom.h \
    googleatom_global.h \
    googlecontacts_global.h


target.path = /usr/lib/buteo-plugins-qt5

sync.path = /etc/buteo/profiles/sync
sync.files = xml/sync/*

client.path = /etc/buteo/profiles/client
client.files = xml/client/*

INSTALLS += target sync client
