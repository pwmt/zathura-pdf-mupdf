# See LICENSE file for license and copyright information

include config.mk

PLUGIN   = pdf
SOURCE   = pdf.c
OBJECTS  = ${SOURCE:.c=.o}
DOBJECTS = ${SOURCE:.c=.do}

ifneq "$(WITH_CAIRO)" "0"
CPPFLAGS += -DHAVE_CAIRO
endif

all: options ${PLUGIN}

options:
	@echo ${PLUGIN} build options:
	@echo "CFLAGS  = ${CFLAGS}"
	@echo "LDFLAGS = ${LDFLAGS}"
	@echo "DFLAGS  = ${DFLAGS}"
	@echo "CC      = ${CC}"

%.o: %.c
	@echo CC $<
	@mkdir -p .depend
	@${CC} -c ${CPPFLAGS} ${CFLAGS} -o $@ $< -MMD -MF .depend/$@.dep

%.do: %.c
	@echo CC $<
	@mkdir -p .depend
	@${CC} -c ${CPPFLAGS} ${CFLAGS} ${DFLAGS} -o $@ $< -MMD -MF .depend/$@.dep

${OBJECTS}:  config.mk
${DOBJECTS}: config.mk

${PLUGIN}: ${OBJECTS}
	@echo LD $@
	@${CC} -shared ${LDFLAGS} -o ${PLUGIN}.so $(OBJECTS) ${LIBS}

${PLUGIN}-debug: ${DOBJECTS}
	@echo LD $@
	@${CC} -shared ${LDFLAGS} -o ${PLUGIN}.so $(DOBJECTS) ${LIBS}

clean:
	@rm -rf ${OBJECTS} ${DOBJECTS} $(PLUGIN).so .depend

debug: options ${PLUGIN}-debug

install: all
	@echo installing ${PLUGIN} plugin
	@mkdir -p ${DESTDIR}${PREFIX}/lib/zathura
	@cp -f ${PLUGIN}.so ${DESTDIR}${PREFIX}/lib/zathura

uninstall:
	@echo uninstalling ${PLUGIN} plugin
	@rm -f ${DESTDIR}${PREFIX}/lib/zathura/${PLUGIN}.so
	@rmdir ${DESTDIR}${PREFIX}/lib/zathura 2> /dev/null || /bin/true

-include $(wildcard .depend/*.dep)

.PHONY: all options clean debug install uninstall
