SOURCES += \
    ser_FHE.cpp


HEADERS += \
    sock.h

unix:!macx:!symbian: LIBS += -L$$PWD/../../../../../usr/local/fhe/lib/ -lfhe

INCLUDEPATH += $$PWD/../../../../../usr/local/fhe/include
DEPENDPATH += $$PWD/../../../../../usr/local/fhe/include

unix:!macx:!symbian: PRE_TARGETDEPS += $$PWD/../../../../../usr/local/fhe/lib/libfhe.a

unix:!macx:!symbian: LIBS += -L$$PWD/../../../../../usr/local/sw/lib/ -lntl

INCLUDEPATH += $$PWD/../../../../../usr/local/sw/include
DEPENDPATH += $$PWD/../../../../../usr/local/sw/include

unix:!macx:!symbian: PRE_TARGETDEPS += $$PWD/../../../../../usr/local/sw/lib/libntl.a




unix:!macx:!symbian: LIBS += -L$$PWD/../../../../../usr/lib/mysql/ -lmysqlclient

INCLUDEPATH += $$PWD/../../../../../usr/lib/mysql
DEPENDPATH += $$PWD/../../../../../usr/lib/mysql
