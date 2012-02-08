# See LICENSE file for license and copyright information

include config.mk
include common.mk

PROJECT  = zathura-pdf-mupdf
PLUGIN   = pdf
SOURCE   = $(shell find . -iname "*.c")
HEADER   = $(shell find . -iname "*.h")
OBJECTS  = ${SOURCE:.c=.o}
DOBJECTS = ${SOURCE:.c=.do}

ifneq "$(WITH_CAIRO)" "0"
CPPFLAGS += -DHAVE_CAIRO
endif

all: options ${PLUGIN}

options:
	$(ECHO) ${PLUGIN} build options:
	$(ECHO) "CFLAGS  = ${CFLAGS}"
	$(ECHO) "LDFLAGS = ${LDFLAGS}"
	$(ECHO) "DFLAGS  = ${DFLAGS}"
	$(ECHO) "CC      = ${CC}"

%.o: %.c
	$(ECHO) CC $<
	@mkdir -p .depend
	$(QUIET)${CC} -c ${CPPFLAGS} ${CFLAGS} -o $@ $< -MMD -MF .depend/$@.dep

%.do: %.c
	$(ECHO) CC $<
	@mkdir -p .depend
	$(QUIET)${CC} -c ${CPPFLAGS} ${CFLAGS} ${DFLAGS} -o $@ $< -MMD -MF .depend/$@.dep

${OBJECTS}:  config.mk
${DOBJECTS}: config.mk

${PLUGIN}: ${OBJECTS}
	$(ECHO) LD $@
	$(QUIET)${CC} -shared ${LDFLAGS} -o ${PLUGIN}.so $(OBJECTS) ${LIBS}

${PLUGIN}-debug: ${DOBJECTS}
	$(ECHO) LD $@
	$(QUIET)${CC} -shared ${LDFLAGS} -o ${PLUGIN}.so $(DOBJECTS) ${LIBS}

clean:
	$(QUIET)rm -rf ${OBJECTS} ${DOBJECTS} $(PLUGIN).so doc .depend \
		${PROJECT}-${VERSION}.tar.gz

debug: options ${PLUGIN}-debug

dist: clean
	$(QUIET)mkdir -p ${PROJECT}-${VERSION}
	$(QUIET)cp -R LICENSE Makefile config.mk common.mk Doxyfile \
		${HEADER} ${SOURCE} ${PROJECT}-${VERSION}
	$(QUIET)tar -cf ${PROJECT}-${VERSION}.tar ${PROJECT}-${VERSION}
	$(QUIET)gzip ${PROJECT}-${VERSION}.tar
	$(QUIET)rm -rf ${PROJECT}-${VERSION}

doc: clean
	$(QUIET)doxygen Doxyfile

install: all
	$(ECHO) installing ${PLUGIN} plugin
	$(QUIET)mkdir -p ${DESTDIR}${PREFIX}/lib/zathura
	$(QUIET)cp -f ${PLUGIN}.so ${DESTDIR}${PREFIX}/lib/zathura

uninstall:
	$(ECHO) uninstalling ${PLUGIN} plugin
	$(QUIET)rm -f ${DESTDIR}${PREFIX}/lib/zathura/${PLUGIN}.so
	$(QUIET)rmdir ${DESTDIR}${PREFIX}/lib/zathura 2> /dev/null || /bin/true

-include $(wildcard .depend/*.dep)

.PHONY: all options clean debug doc dist install uninstall
