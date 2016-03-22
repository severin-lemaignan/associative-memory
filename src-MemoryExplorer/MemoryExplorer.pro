#-------------------------------------------------
#
# Project created by QtCreator 2016-03-22T11:59:23
#
#-------------------------------------------------

QMAKE_CXXFLAGS += -std=c++14

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

CONFIG += link_pkgconfig
PKGCONFIG += eigen3

TARGET = MemoryExplorer
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    qcustomplot.cpp \
    ../src/memory_network.cpp \
    ../src-runner/experiment.cpp

HEADERS  += mainwindow.h \
    qcustomplot.h \
    ../src/memory_network.hpp \
    ../src-runner/parser.hpp \
    ../src-runner/experiment.hpp

FORMS    += mainwindow.ui
