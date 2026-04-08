# The hiha programming language
#
# Copyright © 2026 Barry Schwartz
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

.DELETE_ON_ERROR:

SUBDIRS = lib . # tests

ACLOCAL_AMFLAGS = -I m4 --install

AUTOMAKE_OPTIONS =
AUTOMAKE_OPTIONS += foreign
AUTOMAKE_OPTIONS += subdir-objects

@GNU@ include $(top_srcdir)/silent-rules.mk

CLEANFILES =
DISTCLEANFILES =
EXTRA_DIST =
EXTRA_PROGRAMS =
EXTRA_SCRIPTS =
BUILT_SOURCES =
AM_CPPFLAGS =
AM_CFLAGS =

EXTRA_DIST += m4/gnulib-cache.m4
EXTRA_DIST += silent-rules.mk
EXTRA_DIST += COPYING
EXTRA_DIST += README.adoc
EXTRA_DIST += INSTALL
EXTRA_DIST += AUTHORS

DISTCLEANFILES += Makefile
DISTCLEANFILES += GNUmakefile

BASICCFLAGS =
BASICCFLAGS += -I $(builddir)
BASICCFLAGS += -I $(builddir)/include
BASICCFLAGS += -I $(builddir)/lib
BASICCFLAGS += -I $(srcdir)
BASICCFLAGS += -I $(srcdir)/include
BASICCFLAGS += -I $(srcdir)/lib

#---------------------------------------------------------------------

bin_PROGRAMS = hiha

hiha_LDFLAGS =
hiha_LDADD =
hiha_CFLAGS =
hiha_DEPENDENCIES =
hiha_SOURCES =

hiha_LDFLAGS += -lm

hiha_LDADD += libhiha.a
hiha_LDADD += lib/libgnu.a
hiha_LDADD += $(HARD_LOCALE_LIB)
hiha_LDADD += $(LIBC32CONV)
hiha_LDADD += $(LIBICONV)
hiha_LDADD += $(LIBINTL)
hiha_LDADD += $(LIBUNISTRING)
hiha_LDADD += $(MBRTOWC_LIB)
hiha_LDADD += $(SETLOCALE_NULL_LIB)

hiha_CFLAGS += $(BASICCFLAGS)

hiha_DEPENDENCIES += config.h
hiha_DEPENDENCIES += libhiha.a
hiha_DEPENDENCIES += lib/libgnu.a

hiha_SOURCES += src/hiha/hiha.c

#---------------------------------------------------------------------

noinst_LIBRARIES = libhiha.a

libhiha_a_CFLAGS =
libhiha_a_DEPENDENCIES =
libhiha_a_SOURCES =

libhiha_a_CFLAGS += $(BASICCFLAGS)

libhiha_a_DEPENDENCIES += config.h

libhiha_a_SOURCES += include/libhiha/string_t.h
libhiha_a_SOURCES += include/libhiha/token_t.h
libhiha_a_SOURCES += include/libhiha/parse_expression.h

libhiha_a_SOURCES += src/libhiha/string_t.c
libhiha_a_SOURCES += src/libhiha/token_t.c
libhiha_a_SOURCES += src/libhiha/parse_expression.c

#---------------------------------------------------------------------

.PHONY: indent
indent:
	$(INDENT) -l72 \
		-Tbinding_power_t \
		-Tstring_t \
		-Ttoken_t \
		-Ttext_location_t \
		-Ttoken_getter_from_file_t \
		$(addprefix $(top_srcdir)/, \
			$(filter %.c %.h,$(libhiha_a_SOURCES))) \
		$(addprefix $(top_srcdir)/, \
			$(filter %.c %.h,$(hiha_SOURCES)))

#---------------------------------------------------------------------
# local variables:
# coding: utf-8
# tab-width: 4
# end:
