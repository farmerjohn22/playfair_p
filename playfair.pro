TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += c++17

QMAKE_CXXFLAGS_WARN_ON += \
    -Wall \
    -Wextra \
    -Wpedantic \
    -Wconversion \
    -Wsign-conversion

SOURCES += \
    main.cpp

HEADERS += \
    dict.h \
    simple.h \
    playfair.h \
    chaotic.h

