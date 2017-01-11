# See LICENSE file for license and copyright information

VERSION_MAJOR = 0
VERSION_MINOR = 3
VERSION_REV = 1
VERSION = ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REV}

# minimum required zathura version
ZATHURA_MIN_VERSION = 0.2.0

ZATHURA_VERSION_CHECK ?= $(shell pkg-config --atleast-version=$(ZATHURA_MIN_VERSION) zathura; echo $$?)
ZATHURA_GTK_VERSION ?= $(shell pkg-config --variable=GTK_VERSION zathura)

# paths
PREFIX ?= /usr
LIBDIR ?= ${PREFIX}/lib
DESKTOPPREFIX ?= ${PREFIX}/share/applications

# libs
GTK_INC ?= $(shell pkg-config --cflags gtk+-${ZATHURA_GTK_VERSION}.0)
GTK_LIB ?= $(shell pkg-config --libs gtk+-${ZATHURA_GTK_VERSION}.0)

GIRARA_INC ?= $(shell pkg-config --cflags girara-gtk${ZATHURA_GTK_VERSION})
GIRARA_LIB ?= $(shell pkg-config --libs girara-gtk${ZATHURA_GTK_VERSION})

ZATHURA_INC ?= $(shell pkg-config --cflags zathura)
PLUGINDIR ?= $(shell pkg-config --variable=plugindir zathura)
ifeq (,${PLUGINDIR})
PLUGINDIR = ${LIBDIR}/zathura
endif

OPENSSL_INC ?= $(shell pkg-config --cflags libcrypto)
OPENSSL_LIB ?= $(shell pkg-config --libs libcrypto)

FREETYPE_INC ?= $(shell pkg-config --cflags freetype2)
FREETYPE_LIB ?= $(shell pkg-config --libs freetype2)

HARFBUZZ_INC ?= $(shell pkg-config --cflags harfbuzz)
HARFBUZZ_LIB ?= $(shell pkg-config --libs harfbuzz)

MUPDF_LIB ?= -lmupdf -lmupdfthird

INCS = ${GTK_INC} ${GIRARA_INC} ${OPENSSL_INC} ${ZATHURA_INC} ${FREETYPE_INC} ${HARFBUZZ_INC}
LIBS = ${GTK_LIB} ${GIRARA_LIB} ${MUPDF_LIB} ${OPENSSL_LIB} ${FREETYPE_LIB} ${HARFBUZZ_LIB} -ljbig2dec -lopenjp2 -ljpeg -lz

# compiler flags
CFLAGS += -std=c99 -fPIC -pedantic -Wall -Wno-format-zero-length $(INCS)

# linker flags
LDFLAGS += -fPIC

# debug
DFLAGS ?= -g

# build with cairo support?
WITH_CAIRO ?= 1

# compiler
CC ?= gcc
LD ?= ld

# set to something != 0 if you want verbose build output
VERBOSE ?= 0
