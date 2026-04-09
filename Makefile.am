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

hiha_LDADD += libhiha.la
hiha_LDADD += lib/libgnu.la
hiha_LDADD += $(LTLIBICONV)
hiha_LDADD += $(LTLIBINTL)
hiha_LDADD += $(LTLIBUNISTRING)

hiha_CFLAGS += $(BASICCFLAGS)

hiha_DEPENDENCIES += config.h
hiha_DEPENDENCIES += libhiha.la
hiha_DEPENDENCIES += lib/libgnu.la

hiha_SOURCES += src/hiha/hiha.c

#---------------------------------------------------------------------

lib_LTLIBRARIES = libhiha.la

libhiha_la_LIBADD =
libhiha_la_LDFLAGS =
libhiha_la_CFLAGS =
libhiha_la_DEPENDENCIES =
libhiha_la_SOURCES =

libhiha_la_LIBADD += lib/libgnu.la

libhiha_la_LDFLAGS += $(LTLIBICONV)
libhiha_la_LDFLAGS += $(LTLIBINTL)
libhiha_la_LDFLAGS += $(LTLIBUNISTRING)
libhiha_la_LDFLAGS += -lm

libhiha_la_CFLAGS += $(BASICCFLAGS)

libhiha_la_DEPENDENCIES += config.h

libhiha_la_SOURCES += include/libhiha/string_t.h
libhiha_la_SOURCES += include/libhiha/token_t.h
libhiha_la_SOURCES += include/libhiha/parse_expression.h

libhiha_la_SOURCES += src/libhiha/string_t.c
libhiha_la_SOURCES += src/libhiha/token_t.c
libhiha_la_SOURCES += src/libhiha/parse_expression.c

#---------------------------------------------------------------------

.PHONY: indent
indent:
	$(INDENT) -l72 \
		-TFILE \
		-Tbinding_power_t \
		-Tstring_t \
		-Ttoken_t \
		-Ttext_location_t \
		-Ttoken_getter_t \
		-Ttoken_getter_from_file_t \
		$(addprefix $(top_srcdir)/, \
			$(filter %.c %.h,$(libhiha_la_SOURCES))) \
		$(addprefix $(top_srcdir)/, \
			$(filter %.c %.h,$(hiha_SOURCES)))

#---------------------------------------------------------------------
# local variables:
# coding: utf-8
# tab-width: 4
# end:
