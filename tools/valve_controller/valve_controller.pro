CONFIG += debug
QMAKE_CXXFLAGS += -std=c++11
QT -= core gui

# this will force the makefile to use colorgcc, a wrapper around gcc that colorizes content.
# install on ubuntu with "sudo apt-get install colorgcc"
# or comment out to use straight gcc.
QMAKE_CXX = colorgcc

INCLUDEPATH += include/ \
		../../core/include \
		../../../roller/include \
		../../../devman/include \
		../../../owfs_devman/include \
		../../../raspi_gpio_devman/include \

LIBS += -lboost_program_options \

debug:LIBS += -L../../core/debug/ -lab2_core \
		-L../../../roller/core/debug/ -lroller_core \
		-L../../../devman/debug/ -ldevman \
		-L../../../owfs_devman/debug/ -lowfs_devman \
		-L../../../raspi_gpio_devman/debug/ -lraspi_gpio_devman \

release:LIBS +=  -L../../core/release/ -lab2_core \
		-L../../../roller/core/release/ -lroller_core \
		-L../../../devman/release/ -ldevman \
		-L../../../owfs_devman/release/ -lowfs_devman \
		-L../../../raspi_gpio_devman/release/ -lraspi_gpio_devman \

SOURCES = $$files(src/*.cpp) \

HEADERS = $$files(include/*.h) \

QMAKE_RPATHDIR += "../../core/debug/" \
		"../../../roller/core/debug/" \
		"../../../devman/debug/" \
		"../../../owfs_devman/debug/" \
		"../../../raspi_gpio_devman/debug/" \

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
