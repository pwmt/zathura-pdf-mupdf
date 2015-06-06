# See LICENSE file for license and copyright information

PROJECT  = zathura-pdf-mupdf
PLUGIN   = pdf

VERSION_MAJOR = 0
VERSION_MINOR = 2
VERSION_REV = 5
VERSION = ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REV}

# minimum required zathura version
LIBZATHURA_MIN_VERSION = 0.0.1
LIBZATHURA_VERSION_CHECK ?= $(shell pkg-config --atleast-version=$(LIBZATHURA_MIN_VERSION) libzathura; echo $$?)
LIBZATHURA_VERSION_CHECK_FILE = .libzathura-version-check

# paths
PREFIX ?= /usr
LIBDIR ?= ${PREFIX}/lib
DEPENDDIR=.depend
BUILDDIR=build
BUILDDIR_RELEASE=${BUILDDIR}/release
BUILDDIR_DEBUG=${BUILDDIR}/debug
BUILDDIR_GCOV=${BUILDDIR}/gcov
DESKTOPPREFIX ?= ${PREFIX}/share/applications
PLUGINDIR ?= $(shell pkg-config --variable=plugindir libzathura)
TARFILE = ${PROJECT}-${VERSION}.tar.gz
TARDIR = ${PROJECT}-${VERSION}

# libs
CAIRO_INC ?= $(shell pkg-config --cflags cairo)
CAIRO_LIB ?= $(shell pkg-config --libs cairo)

OPENSSL_INC ?= $(shell pkg-config --cflags libcrypto)
OPENSSL_LIB ?= $(shell pkg-config --libs libcrypto)

LIBZATHURA_INC ?= $(shell pkg-config --cflags libzathura)
LIBZATHURA_LIB ?= $(shell pkg-config --libs libzathura)

MUPDF_LIB ?= -lmupdf -lmujs

FIU_INC ?= $(shell pkg-config --cflags libfiu)
FIU_LIB ?= $(shell pkg-config --libs libfiu)

INCS = ${OPENSSL_INC} ${LIBZATHURA_INC}
LIBS = ${MUPDF_LIB} ${OPENSSL_LIB} ${LIBZATHURA_LIB} -ljbig2dec -lopenjp2 -ljpeg

# flags
CFLAGS += -std=c11 -pedantic -Wall -Wextra -fPIC --coverage $(INCS)

# linker flags
LDFLAGS += -fPIC --coverage

# debug
DFLAGS ?= -g

# compiler
CC ?= gcc

# strip
SFLAGS ?= -s

# gcov & lcov
GCOV_CFLAGS=-fprofile-arcs -ftest-coverage
GCOV_LDFLAGS=-fprofile-arcs
LCOV_OUTPUT=gcov
LCOV_EXEC=lcov
LCOV_FLAGS=--base-directory . --directory ${BUILDDIR_GCOV} --capture --rc \
					 lcov_branch_coverage=1 --output-file ${BUILDDIR_GCOV}/$(PROJECT).info
GENHTML_EXEC=genhtml
GENHTML_FLAGS=--rc lcov_branch_coverage=1 --output-directory ${LCOV_OUTPUT} ${BUILDDIR_GCOV}/$(PROJECT).info

# cairo support
WITH_CAIRO ?= 1

# libfiu
WITH_LIBFIU ?= 1
FIU_RUN ?= fiu-run -x

# set to something != 0 if you want verbose build output
VERBOSE ?= 0

# enable colors
COLOR ?= 1
