######################################################################
# Automatically generated by qmake (2.01a) Tue Oct 15 17:17:01 2013
######################################################################

TEMPLATE = app
TARGET = bin/ringer
DEPENDPATH += . include src
INCLUDEPATH += . include
INCLUDEPATH += /usr/include/cryptopp/
INCLUDEPATH += /usr/include/ring/
INCLUDEPATH += /usr/include/sha/
INCLUDEPATH += /usr/include/prng/
QMAKE_LIBDIR += /usr/lib/
LIBS += -lring
LIBS += -lprng
LIBS += -lsha
LIBS += -lcrypto++
OBJECTS_DIR = obj

# Input
SOURCES += src/ringer.cpp
