PROG_CXX=doorbell
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
	@echo Install

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
