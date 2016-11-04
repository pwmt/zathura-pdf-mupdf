# See LICENSE file for license and copyright information

include config.mk
include common.mk

PROJECT  = zathura-pdf-mupdf
PLUGIN   = pdf
SOURCE   = $(sort $(wildcard *.c))
HEADER   = $(sort $(wildcard *.h))
OBJECTS  = ${SOURCE:.c=.o}
DOBJECTS = ${SOURCE:.c=.do}

ifneq "$(WITH_CAIRO)" "0"
CPPFLAGS += -DHAVE_CAIRO
endif

CPPFLAGS += "-DVERSION_MAJOR=${VERSION_MAJOR}"
CPPFLAGS += "-DVERSION_MINOR=${VERSION_MINOR}"
CPPFLAGS += "-DVERSION_REV=${VERSION_REV}"

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
		${HEADER} ${SOURCE} AUTHORS ${PROJECT}.desktop \
		${PROJECT}-${VERSION}
	$(QUIET)tar -cf ${PROJECT}-${VERSION}.tar ${PROJECT}-${VERSION}
	$(QUIET)gzip ${PROJECT}-${VERSION}.tar
	$(QUIET)rm -rf ${PROJECT}-${VERSION}

doc: clean
	$(QUIET)doxygen Doxyfile

install: all
	$(ECHO) installing ${PLUGIN} plugin
	$(QUIET)mkdir -p ${DESTDIR}${PLUGINDIR}
	$(QUIET)cp -f ${PLUGIN}.so ${DESTDIR}${PLUGINDIR}
	$(QUIET)mkdir -m 755 -p ${DESTDIR}${DESKTOPPREFIX}
	$(ECHO) installing desktop file
	$(QUIET)install -m 644 ${PROJECT}.desktop ${DESTDIR}${DESKTOPPREFIX}

uninstall:
	$(ECHO) uninstalling ${PLUGIN} plugin
	$(QUIET)rm -f ${DESTDIR}${PLUGINDIR}/${PLUGIN}.so
	$(QUIET)rmdir --ignore-fail-on-non-empty ${DESTDIR}${PLUGINDIR} 2> /dev/null
	$(ECHO) removing desktop file
	$(QUIET)rm -f ${DESTDIR}${DESKTOPPREFIX}/${PROJECT}.desktop
	$(QUIET)rmdir --ignore-fail-on-non-empty ${DESTDIR}${DESKTOPPREFIX} 2> /dev/null

-include $(wildcard .depend/*.dep)

.PHONY: all options clean debug doc dist install uninstall
