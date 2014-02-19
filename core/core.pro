TEMPLATE = lib
TARGET = ab2_core

CONFIG += debug
QMAKE_CXXFLAGS += -std=c++0x
QT -= core gui

# this will force the makefile to use colorgcc, a wrapper around gcc that colorizes content.
# install on ubuntu with "sudo apt-get install colorgcc"
# or comment out to use straight gcc.
QMAKE_CXX = colorgcc

INCLUDEPATH += include/ \
		../../roller/include \
		../../devman/include \

debug:LIBS += -L../../roller/core/debug/ -lroller_core \
		-L../../devman/debug/ -ldevman \

release:LIBS += -L../../roller/core/release/ -lroller_core \
		-L../../devman/release/ -ldevman \

SOURCES = $$files(src/*.cpp) \

HEADERS = $$files(include/*.h) \

QMAKE_RPATHDIR += "../../roller/core/debug/" \
		"../../devman/debug/" \

release:DESTDIR = release
release:OBJECTS_DIR = release/.obj
release:MOC_DIR = release/.moc
release:RCC_DIR = release/.rcc
release:UI_DIR = release/.ui

debug:DESTDIR = debug
debug:OBJECTS_DIR = debug/.obj
debug:MOC_DIR = debug/.moc
debug:RCC_DIR = debug/.rcc
debug:UI_DIR = debug/.ui
