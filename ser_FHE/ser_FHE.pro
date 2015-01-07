SOURCES += \
    ser_FHE.cpp


HEADERS += \
    sock.h

unix:!macx:!symbian: LIBS += -L$$PWD/../libfhe.a/ -lfhe

INCLUDEPATH += $$PWD/../libfhe.a
DEPENDPATH += $$PWD/../libfhe.a

unix:!macx:!symbian: PRE_TARGETDEPS += $$PWD/../libfhe.a/libfhe.a

unix:!macx:!symbian: LIBS += -L$$PWD/../../../../../root/sw/lib/ -lntl

INCLUDEPATH += $$PWD/../../../../../root/sw/include
DEPENDPATH += $$PWD/../../../../../root/sw/include

unix:!macx:!symbian: PRE_TARGETDEPS += $$PWD/../../../../../root/sw/lib/libntl.a

unix:!macx:!symbian: LIBS += -L$$PWD/../../../../../usr/lib/mysql/ -lmysqlclient

INCLUDEPATH += $$PWD/../../../../../usr/lib/mysql
DEPENDPATH += $$PWD/../../../../../usr/lib/mysql
