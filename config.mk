# See LICENSE file for license and copyright information

# paths
PREFIX ?= /usr

# libs
GTK_INC ?= $(shell pkg-config --cflags gtk+-2.0)
GTK_LIB ?= $(shell pkg-config --libs gtk+-2.0)

GIRARA_INC ?= $(shell pkg-config --cflags girara-gtk2)
ZATHURA_INC ?= $(shell pkg-config --cflags zathura)

INCS = -I. -I/usr/include ${GTK_INC} ${ZATHURA_INC} ${GIRARA_INC}
LIBS = -lc ${GTK_LIB} -lmupdf -ljbig2dec -ljpeg -lopenjpeg -lfitz

# flags
CFLAGS += -std=c99 -fPIC -pedantic -Wall -Wno-format-zero-length $(INCS)

# debug
DFLAGS ?= -g

# build with cairo support?
WITH_CAIRO ?= 1

# compiler
CC ?= gcc
LD ?= ld
