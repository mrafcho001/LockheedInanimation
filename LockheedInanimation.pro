#-------------------------------------------------
#
# Project created by QtCreator 2013-02-16T12:22:38
#
#-------------------------------------------------

QT       += core gui svg opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = LockheedInanimation
TEMPLATE = app

LIBS += -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_contrib -lopencv_legacy -lopencv_flann

SOURCES += src/main.cpp\
        src/mainwindow.cpp \
    src/facetracker.cpp \
    src/faceinvaderswidget.cpp

HEADERS  += src/mainwindow.h \
    src/facetracker.h \
    src/faceinvaderswidget.h

FORMS    += resources/mainwindow.ui

ARDUINO_SOURCES += src/Arduino/arduino_sketch.ino

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

QMAKE_EXTRA_TARGETS += docs arduino

RESOURCES += \
    resources/resources.qrc

OTHER_FILES += \
    docs/mainpage.dox \
    docs/explanations.dox

DEFINES +=  #DEBUG_FACETRACKING_TIMING=1
DEFINES +=  #DEBUG_CAPTURE_TIMING=1
DEFINES +=  DEBUG_REPORT_FPS=1
DEFINES +=  #DEBUG_INVADER_SHAPE=1

#Arduino Sketch
arduino.depends = $(ARDUINO_SOURCES)
arduino.commands = make -C src/Arduino/

