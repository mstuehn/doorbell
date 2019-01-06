.include <bsd.init.mk>

DTS=ti_pruss.dts

.SUFFIXES	:  .dtbo .dts

DTB=${DTS:R:S/$/.dtbo/}

all: ${DTB}

.if defined(DTS)
.for _dts in ${DTS}
${_dts:R:S/$/.dtbo/}:	${_dts} ${OP_META}
	@echo Generating ${.TARGET} from ${_dts}
	dtc -@ -Idts -Odtb -o ${.TARGET} ${_dts}
CLEANFILES+=${_dts:R:S/$/.dtbo/}
.endfor
.endif

#.p.bin		:
#	${ASSEMBLER} ${FLAGS} ${.IMPSRC}
#
#clean:
#	rm -f ${PROG} gpio.txt gpio_bin.h
#
#program:
#	${PRUCTL} -t ti -p 0 -re gpio.bin

install: ${DTB}
	@echo "Overlay-Install"
	@echo install ${DTB} /boot/dtb/overlays/

.include <bsd.dep.mk>
.include <bsd.obj.mk>
.include <bsd.links.mk>
