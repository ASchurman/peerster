######################################################################
# Automatically generated by qmake (2.01a) Mon Aug 27 15:02:08 2012
######################################################################

TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .
QT += network

CONFIG += crypto

# Input
SOURCES += main.cc \
    Search.cc

HEADERS += Common.hh \
    Search.hh

HEADERS += NetSocket.hh
SOURCES += NetSocket.cc

HEADERS += ChatDialog.hh
SOURCES += ChatDialog.cc

HEADERS += MessageStore.hh
SOURCES += MessageStore.cc

HEADERS += Monger.hh
SOURCES += Monger.cc

HEADERS += RouteTable.hh
SOURCES += RouteTable.cc

HEADERS += FileData.hh
SOURCES += FileData.cc

HEADERS += FileStore.hh
SOURCES += FileStore.cc
