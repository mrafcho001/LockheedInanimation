#-------------------------------------------------
#
# Project created by QtCreator 2013-02-16T12:22:38
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = LockheedInanimation
TEMPLATE = app

LIBS += -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_contrib -lopencv_legacy -lopencv_flann

SOURCES += src/main.cpp\
        src/mainwindow.cpp \
    src/facetracker.cpp

HEADERS  += src/mainwindow.h \
    src/facetracker.h

FORMS    += resources/mainwindow.ui

OBJECTS_DIR = build/.obj
MOC_DIR = build/.moc
RCC_DIR = build/.rcc
UI_DIR = build/.ui

CONFIG(debug, debug|release){
    DESTDIR = build/debug

    CONFIG -= release
    CONFIG += debug
    QMAKE_CXXFLAGS_RELEASE += -g
    QMAKE_CFLAGS_RELEASE += -g
}
else {
    DESTDIR = build/release

    CONFIG += release
    CONFIG -= debug
}

QMAKE_CLEAN += Makefile -r build/.[a-z]* build/*

#Doxygen
docs.depends = $(SOURCES)
docs.commands = doxygen Doxyfile

QMAKE_EXTRA_TARGETS += docs

RESOURCES += \
    resources/resources.qrc

