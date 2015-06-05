# See LICENSE file for license and copyright information

include config.mk
include colors.mk
include common.mk

SOURCE        = $(wildcard ${PROJECT}/*.c)
HEADER        = $(wildcard ${PROJECT}/*.h)
OBJECTS       = $(addprefix ${BUILDDIR_RELEASE}/,${SOURCE:.c=.o})
OBJECTS_DEBUG = $(addprefix ${BUILDDIR_DEBUG}/,${SOURCE:.c=.o})
OBJECTS_GCOV  = $(addprefix ${BUILDDIR_GCOV}/,${SOURCE:.c=.o})

ifneq (${WITH_CAIRO},0)
INCS += ${CAIRO_INC}
LIBS += ${CAIRO_LIB}
CPPFLAGS += -DHAVE_CAIRO
endif

CPPFLAGS += "-DVERSION_MAJOR=${VERSION_MAJOR}"
CPPFLAGS += "-DVERSION_MINOR=${VERSION_MINOR}"
CPPFLAGS += "-DVERSION_REV=${VERSION_REV}"

all: options ${BUILDDIR_RELEASE}/${PLUGIN}.so

options:
ifeq "$(VERBOSE)" "1"
	$(ECHO) ${PROJECT} build options:
	$(ECHO) "CFLAGS  = ${CFLAGS}"
	$(ECHO) "LDFLAGS = ${LDFLAGS}"
	$(ECHO) "DFLAGS  = ${DFLAGS}"
	$(ECHO) "CC      = ${CC}"
endif

# check libzathura version

${LIBZATHURA_VERSION_CHECK_FILE}:
ifneq ($(LIBZATHURA_VERSION_CHECK), 0)
	$(error "The minimum required version of libzathura is ${LIBZATHURA_MIN_VERSION}")
endif
	$(QUIET)touch ${LIBZATHURA_VERSION_CHECK_FILE}

# release build

${OBJECTS}: config.mk ${LIBZATHURA_VERSION_CHECK_FILE}

${BUILDDIR_RELEASE}/%.o: %.c
	$(call colorecho,CC,$<)
	@mkdir -p ${DEPENDDIR}/$(dir $(abspath $@))
	@mkdir -p $(dir $(abspath $@))
	$(QUIET)${CC} -c ${CPPFLAGS} ${CFLAGS} \
		-o $@ $< -MMD -MF ${DEPENDDIR}/$(abspath $@).dep

${BUILDDIR_RELEASE}/${PLUGIN}.so: ${OBJECTS}
	$(call colorecho,LD,$@)
	$(QUIET)${CC} -shared ${LDFLAGS} \
		-o ${BUILDDIR_RELEASE}/${PLUGIN}.so ${OBJECTS} ${LIBS}

# debug build

${OBJECTS_DEBUG}: config.mk ${LIBZATHURA_VERSION_CHECK_FILE}

${BUILDDIR_DEBUG}/%.o: %.c
	$(call colorecho,CC,$<)
	@mkdir -p ${DEPENDDIR}/$(dir $(abspath $@))
	@mkdir -p $(dir $(abspath $@))
	$(QUIET)${CC} -c ${CPPFLAGS} ${CFLAGS} ${DFLAGS} \
		-o $@ $< -MMD -MF ${DEPENDDIR}/$(abspath $@).dep

${BUILDDIR_DEBUG}/${PLUGIN}.so: ${OBJECTS_DEBUG}
	$(call colorecho,LD,${PLUGIN}.so)
	$(QUIET)${CC} -shared ${LDFLAGS} \
		-o ${BUILDDIR_DEBUG}/${PLUGIN}.so ${OBJECTS_DEBUG} ${LIBS}

debug: options ${BUILDDIR_DEBUG}/${PLUGIN}.so

# gcov build

${OBJECTS_GCOV}: config.mk ${LIBZATHURA_VERSION_CHECK_FILE}

${BUILDDIR_GCOV}/%.o: %.c
	$(call colorecho,CC,$<)
	@mkdir -p ${DEPENDDIR}/$(dir $(abspath $@))
	@mkdir -p $(dir $(abspath $@))
	$(QUIET)${CC} -c ${CPPFLAGS} ${CFLAGS} ${DFLAGS} \
		-o $@ $< -MMD -MF ${DEPENDDIR}/$(abspath $@).dep

${BUILDDIR_GCOV}/${PLUGIN}.so: ${OBJECTS_GCOV}
	$(call colorecho,LD,${PLUGIN}.so)
	$(QUIET)${CC} -shared ${LDFLAGS} \
		-o ${BUILDDIR_GCOV}/${PLUGIN}.so ${OBJECTS_GCOV} ${LIBS}

gcov: options ${BUILDDIR_GCOV}/${PLUGIN}.so
	$(QUIET)${MAKE} -C tests run-gcov
	$(call colorecho,LCOV,"Analyse data")
	$(QUIET)${LCOV_EXEC} ${LCOV_FLAGS}
	$(call colorecho,LCOV,"Generate report")
	$(QUIET)${GENHTML_EXEC} ${GENHTML_FLAGS}

# clean

clean:
	$(call colorecho,RM, "Clean objects and builds")
	$(QUIET)rm -rf ${BUILDDIR}
	$(QUIET)rm -f ${LIBZATHURA_VERSION_CHECK_FILE}

	$(call colorecho,RM, "Clean dependencies")
	$(QUIET)rm -rf ${DEPENDDIR}

	$(call colorecho,RM, "Clean distribution files")
	$(QUIET)rm -rf ${PROJECT}-${VERSION}.tar.gz
	$(QUIET)rm -rf ${PROJECT}.info
	$(QUIET)rm -rf ${PROJECT}/version.h

	$(call colorecho,RM, "Clean code analysis")
	$(QUIET)rm -rf ${LCOV_OUTPUT}
	$(QUIET)rm -rf gcov

	$(QUIET)${MAKE} -C tests clean
	$(QUIET)${MAKE} -C doc clean

# documentation

doc:
	$(QUIET)${MAKE} -C doc

# test suite

test: ${PROJECT}
	$(QUIET)${MAKE} -C tests run

# distribution

dist: clean
	$(QUIET)tar -czf $(TARFILE) --exclude=.gitignore \
		--transform 's,^,${PROJECT}-$(VERSION)/,' \
		`git ls-files`

# installation

install: all
	$(call colorecho,INSTALL,"Install plugin file")
	$(QUIET)mkdir -m 755 -p ${DESTDIR}${PLUGINDIR}
	$(QUIET)install -m 644 ${BUILDDIR_RELEASE}/${PLUGIN}.so ${DESTDIR}${PLUGINDIR}
	$(call colorecho,INSTALL,"Install desktop file")
	$(QUIET)mkdir -m 755 -p ${DESTDIR}${DESKTOPPREFIX}
	$(QUIET)install -m 644 ${PROJECT}.desktop ${DESTDIR}${DESKTOPPREFIX}

uninstall:
	$(call colorecho,INSTALL,"Remove plugin file")
	$(QUIET)rm -f ${DESTDIR}${PLUGINDIR}/${PLUGIN}.so
	$(QUIET)rmdir --ignore-fail-on-non-empty ${DESTDIR}${PLUGINDIR} 2> /dev/null
	$(call colorecho,INSTALL,"Remove desktop file")
	$(QUIET)rm -f ${DESTDIR}${DESKTOPPREFIX}/${PROJECT}.desktop
	$(QUIET)rmdir --ignore-fail-on-non-empty ${DESTDIR}${DESKTOPPREFIX} 2> /dev/null

-include $(wildcard .depend/*.dep)

.PHONY: all options clean debug doc test dist install uninstall \
	${PROJECT} ${PROJECT}-gcov.so

${DEPENDDIR}S = ${OBJECTS:.o=.o.dep}
DEPENDS = ${${DEPENDDIR}S:^=${DEPENDDIR}/}
-include $${DEPENDDIR}S}
