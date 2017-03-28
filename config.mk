# See LICENSE file for license and copyright information

VERSION_MAJOR = 0
VERSION_MINOR = 3
VERSION_REV = 1
VERSION = ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REV}

PKG_CONFIG ?= pkg-config

# minimum required zathura version
ZATHURA_MIN_VERSION = 0.2.0

ZATHURA_VERSION_CHECK ?= $(shell $(PKG_CONFIG) --atleast-version=$(ZATHURA_MIN_VERSION) zathura; echo $$?)
ZATHURA_GTK_VERSION ?= $(shell $(PKG_CONFIG) --variable=GTK_VERSION zathura)

# paths
PREFIX ?= /usr
LIBDIR ?= ${PREFIX}/lib
DESKTOPPREFIX ?= ${PREFIX}/share/applications

# libs
CAIRO_INC ?= $(shell $(PKG_CONFIG) --cflags cairo)
CAIRO_LIB ?= $(shell $(PKG_CONFIG) --libs cairo)

GTK_INC ?= $(shell $(PKG_CONFIG) --cflags gtk+-${ZATHURA_GTK_VERSION}.0)
GTK_LIB ?= $(shell $(PKG_CONFIG) --libs gtk+-${ZATHURA_GTK_VERSION}.0)

GIRARA_INC ?= $(shell $(PKG_CONFIG) --cflags girara-gtk${ZATHURA_GTK_VERSION})
GIRARA_LIB ?= $(shell $(PKG_CONFIG) --libs girara-gtk${ZATHURA_GTK_VERSION})

ZATHURA_INC ?= $(shell $(PKG_CONFIG) --cflags zathura)
PLUGINDIR ?= $(shell $(PKG_CONFIG) --variable=plugindir zathura)
ifeq (,${PLUGINDIR})
PLUGINDIR = ${LIBDIR}/zathura
endif

OPENSSL_INC ?= $(shell $(PKG_CONFIG) --cflags libcrypto)
OPENSSL_LIB ?= $(shell $(PKG_CONFIG) --libs libcrypto)

FREETYPE_INC ?= $(shell $(PKG_CONFIG) --cflags freetype2)
FREETYPE_LIB ?= $(shell $(PKG_CONFIG) --libs freetype2)

HARFBUZZ_INC ?= $(shell $(PKG_CONFIG) --cflags harfbuzz)
HARFBUZZ_LIB ?= $(shell $(PKG_CONFIG) --libs harfbuzz)

MUPDF_LIB ?= -lmupdf -lmupdfthird

INCS = ${GTK_INC} ${GIRARA_INC} ${OPENSSL_INC} ${CAIRO_INC} ${ZATHURA_INC} ${FREETYPE_INC} ${HARFBUZZ_INC}
LIBS = ${GTK_LIB} ${GIRARA_LIB} ${MUPDF_LIB} ${OPENSSL_LIB} ${CAIRO_LIB} ${FREETYPE_LIB} ${HARFBUZZ_LIB} -ljbig2dec -lopenjp2 -ljpeg -lz

# compiler flags
CFLAGS += -std=c99 -fPIC -pedantic -Wall -Wno-format-zero-length $(INCS)

# linker flags
LDFLAGS += -fPIC

# debug
DFLAGS ?= -g

# compiler
CC ?= gcc
LD ?= ld

# set to something != 0 if you want verbose build output
VERBOSE ?= 0
