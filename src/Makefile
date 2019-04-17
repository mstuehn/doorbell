PROG_CXX=doorbell
SRCS=main.cpp wavfile.cpp sound.cpp mqtt.cpp
MAN=
PREFIX?=/usr/local

INCLUDES= -I. \
	-I/usr/include \
	-I/usr/local/include \
	-I/usr/local/include/jsoncpp \
	-I/home/manuel/devel/github/libpru

CFLAGS+=${INCLUDES}

CXXFLAGS+=${INCLUDES}

LDFLAGS+=-L/usr/lib \
	 -L/usr/local/lib \
	-L/home/manuel/devel/github/libpru/build

LDADD=-ljsoncpp -lmosquitto -lmosquittopp -lgpio -pthread
COMPILER_FEATURES=c++11

install: all
	@echo Install

.include <bsd.prog.mk>
