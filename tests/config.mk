# See LICENSE file for license and copyright information

GLIB_INC ?= $(shell pkg-config --cflags glib-2.0)
GLIB_LIB ?= $(shell pkg-config --libs glib-2.0)

CHECK_INC ?= $(shell pkg-config --cflags check)
CHECK_LIB ?= $(shell pkg-config --libs check)

INCS += ${GLIB_INC} ${CHECK_INC} ${FIU_INC} -I../zathura-pdf-mupdf
LIBS += ${GLIB_LIB} ${CHECK_LIB} ${FIU_LIB} -Wl,--whole-archive -Wl,--no-whole-archive
LDFLAGS += -rdynamic

PLUGIN_RELEASE=../${BUILDDIR_RELEASE}/${PLUGIN}.so
PLUGIN_DEBUG=../${BUILDDIR_DEBUG}/${PLUGIN}.so
PLUGIN_GCOV=../${BUILDDIR_GCOV}/${PLUGIN}.so

# valgrind
VALGRIND = valgrind
VALGRIND_ARGUMENTS = --tool=memcheck --leak-check=yes --leak-resolution=high \
	--show-reachable=yes --log-file=${PROJECT}-valgrind.log
VALGRIND_SUPPRESSION_FILE = ${PROJECT}.suppression
