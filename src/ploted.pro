################################################################
# Ploted - Plot Editor
# Copyright (C) 2013   Nikita Vadimovich Sadkov
#
# This software is provided only as part of my portfolio.
# All rights belong to ЗАО «НПП «СКИЗЭЛ»
################################################################

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TEMPLATE = app

#don't create bundle on OSX
CONFIG-=app_bundle

ROOT = $${PWD}/..
DESTDIR      = $${ROOT}

TARGET       = ploted


#MacOSX
#LIBS      += -F$${ROOT}/libs
#LIBS += -framework qwt

#Window and Linux
LIBS      += -L$${ROOT}/libs
LIBS      += -lqwt
INCLUDEPATH   += $${ROOT}/libs/qwt

SOURCES = \
    main.cpp

HEADERS += \
    common.h \
    main.h

RESOURCES += resources.qrc

OTHER_FILES += \
    scratch.txt \
    task.txt

#remove the annoyances
QMAKE_CFLAGS_WARN_ON -= -Wall
QMAKE_CXXFLAGS_WARN_ON -= -Wall
#QMAKE_CXXFLAGS += -save-temps

# GCC < 4.7
#QMAKE_CXXFLAGS += "-std=c++0x"

# GCC >= 4.7
#QMAKE_CXXFLAGS += -std=c++11
