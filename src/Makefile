PROG_CXX=bell
SRCS=main.cpp wavfile.cpp comms.cpp pructl.c
MAN=
PREFIX?=/usr/local

INCLUDES= -I. \
	-I/usr/include \
	-I/usr/local/include \
	-I/usr/local/include/jsoncpp

CFLAGS+=${INCLUDES}

CXXFLAGS+=${INCLUDES}

LDFLAGS+=-L/usr/lib \
	 -L/usr/local/lib \

LDADD=-ljsoncpp -lmosquitto -lmosquittopp -lpru -pthread
COMPILER_FEATURES=c++11

FIRMWARE=gpio_bin.h
PRU_SRC=gpio.p
ASSEMBLER=pasm
PRUCTL=pructl
FLAGS=-V3 -bLc -Cpru_gpio_fw
CLEANFILES+=${FIRMWARE} gpio.txt gpio.bin

all: ${FIRMWARE} ${PROG_CXX}

install: all
	install ${PROG_CXX} ${PREFIX}/bin
	mkdir -p ${PREFIX}/share/bell
	install DoorBell.wav ${PREFIX}/share/bell/DoorBell.wav
	install config.json ${PREFIX}/share/bell/config.json

.SUFFIXES	: .p .h

.if defined(PRU_SRC)
.for _prusrc in ${PRU_SRC}
${_prusrc:R:S/$/_bin.h/}:	${_prusrc} ${OP_META}
	@echo Generating ${.TARGET} from ${_prusrc}
	${ASSEMBLER} ${FLAGS} ${_prusrc}
CLEANFILES+=${_prusrc:R:S/$/_bin.h/}
.endfor
.endif

.include <bsd.prog.mk>
