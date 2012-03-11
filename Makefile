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

all: options ${PLUGIN}.so

zathura-version-check:
ifneq ($(ZATHURA_VERSION_CHECK), 0)
	$(error "The minimum required version of zathura is ${ZATHURA_MIN_VERSION}")
endif
	$(QUIET)touch zathura-version-check

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

${OBJECTS}:  config.mk zathura-version-check
${DOBJECTS}: config.mk zathura-version-check

${PLUGIN}.so: ${OBJECTS}
	$(ECHO) LD $@
	$(QUIET)${CC} -shared ${LDFLAGS} -o $@ $(OBJECTS) ${LIBS}

${PLUGIN}-debug.so: ${DOBJECTS}
	$(ECHO) LD $@
	$(QUIET)${CC} -shared ${LDFLAGS} -o $@ $(DOBJECTS) ${LIBS}

clean:
	$(QUIET)rm -rf ${OBJECTS} ${DOBJECTS} $(PLUGIN).so $(PLUGIN)-debug.so \
		doc .depend ${PROJECT}-${VERSION}.tar.gz zathura-version-check

debug: options ${PLUGIN}-debug.so

dist: clean
	$(QUIET)mkdir -p ${PROJECT}-${VERSION}
	$(QUIET)cp -R LICENSE Makefile config.mk common.mk Doxyfile \
		${HEADER} ${SOURCE} AUTHORS ${PROJECT}-${VERSION}
	$(QUIET)tar -cf ${PROJECT}-${VERSION}.tar ${PROJECT}-${VERSION}
	$(QUIET)gzip ${PROJECT}-${VERSION}.tar
	$(QUIET)rm -rf ${PROJECT}-${VERSION}

doc: clean
	$(QUIET)doxygen Doxyfile

install: all
	$(ECHO) installing ${PLUGIN} plugin
	$(QUIET)mkdir -p ${DESTDIR}${PLUGINDIR}
	$(QUIET)cp -f ${PLUGIN}.so ${DESTDIR}${PLUGINDIR}

uninstall:
	$(ECHO) uninstalling ${PLUGIN} plugin
	$(QUIET)rm -f ${DESTDIR}${PLUGINDIR}/${PLUGIN}.so
	$(QUIET)rmdir ${DESTDIR}${PLUGINDIR} 2> /dev/null || /bin/true

-include $(wildcard .depend/*.dep)

.PHONY: all options clean debug doc dist install uninstall
