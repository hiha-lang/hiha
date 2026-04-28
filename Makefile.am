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
AUTOMAKE_OPTIONS += subdir-objects
AUTOMAKE_OPTIONS += -Wno-portability

@GNU@ include $(top_srcdir)/silent-rules.mk

pkg_plugins_libdir = $(pkglibdir)/plugins
pkg_gc_includedir = $(pkgincludedir)/gc
pkg_libhiha_includedir = $(pkgincludedir)/libhiha

bin_PROGRAMS =
lib_LTLIBRARIES =
pkg_plugins_lib_LTLIBRARIES =
pkginclude_HEADERS =
pkg_gc_include_HEADERS =
pkg_libhiha_include_HEADERS =

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
DISTCLEANFILES += lib/Makefile
DISTCLEANFILES += lib/GNUmakefile

BASICCFLAGS =
BASICCFLAGS += -I $(abs_top_builddir)
BASICCFLAGS += -I $(abs_top_builddir)/include
BASICCFLAGS += -I $(abs_top_builddir)/lib
BASICCFLAGS += -I $(abs_top_srcdir)
BASICCFLAGS += -I $(abs_top_srcdir)/include
BASICCFLAGS += -I $(abs_top_srcdir)/lib
BASICCFLAGS += -I $(abs_top_srcdir)/$(GCVERS)/include

BUILT_SOURCES += config.h

#---------------------------------------------------------------------

include gc-dist.am
include 10-eof.am
include 20-white_space.am
include 30-decimal_integer.am

BUILT_SOURCES += $(GCVERS)/libhiha-gc.la

$(GCVERS)/libhiha-gc.la:
	$(MAKE) $(AM_MAKEFLAGS) -C $(GCVERS)

mostlyclean-local::
	-$(MAKE) $(AM_MAKEFLAGS) -C $(GCVERS) mostlyclean

clean-local::
	-$(MAKE) $(AM_MAKEFLAGS) -C $(GCVERS) clean

distclean-local::
	-$(MAKE) $(AM_MAKEFLAGS) -C $(GCVERS) distclean
	-find $(GCVERS) -name 'libcord_la-*.Plo' -exec rm -f '{}' ';'

install-exec-local::
	-mkdir -p '$(DESTDIR)$(libdir)'
	$(LIBTOOL) --mode=install $(INSTALL_PROGRAM) \
	    $(GCVERS)/libhiha-gc.la \
	    $(GCVERS)/libhiha-cord.la \
	    '$(DESTDIR)$(libdir)'

uninstall-local::
	$(LIBTOOL) $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=uninstall \
	    rm -f '$(DESTDIR)$(libdir)'/{libhiha-cord,libhiha-gc}.la

include/gc.h: Makefile.am
	$(call v,GEN)mkdir -p $(@D) && \
	rm -f $(@) && \
	echo '/* This file is installed for backward compatibility. */' > $(@) && \
	echo '#include <gc/gc.h>' >> $(@)

CLEANFILES += include/gc.h

pkginclude_HEADERS += include/gc.h
pkg_gc_include_HEADERS += $(top_srcdir)/$(GCVERS)/include/cord.h
pkg_gc_include_HEADERS += $(top_srcdir)/$(GCVERS)/include/cord_pos.h
pkg_gc_include_HEADERS += $(top_srcdir)/$(GCVERS)/include/ec.h
pkg_gc_include_HEADERS += $(top_srcdir)/$(GCVERS)/include/gc_allocator.h
pkg_gc_include_HEADERS += $(top_srcdir)/$(GCVERS)/include/gc_backptr.h
pkg_gc_include_HEADERS += $(top_srcdir)/$(GCVERS)/include/gc_config_macros.h
pkg_gc_include_HEADERS += $(top_srcdir)/$(GCVERS)/include/gc_cpp.h
pkg_gc_include_HEADERS += $(top_srcdir)/$(GCVERS)/include/gc_disclaim.h
pkg_gc_include_HEADERS += $(top_srcdir)/$(GCVERS)/include/gc_gcj.h
pkg_gc_include_HEADERS += $(top_srcdir)/$(GCVERS)/include/gc.h
pkg_gc_include_HEADERS += $(top_srcdir)/$(GCVERS)/include/gc_inline.h
pkg_gc_include_HEADERS += $(top_srcdir)/$(GCVERS)/include/gc_mark.h
pkg_gc_include_HEADERS += $(top_srcdir)/$(GCVERS)/include/gc_pthread_redirects.h
pkg_gc_include_HEADERS += $(top_srcdir)/$(GCVERS)/include/gc_tiny_fl.h
pkg_gc_include_HEADERS += $(top_srcdir)/$(GCVERS)/include/gc_typed.h
pkg_gc_include_HEADERS += $(top_srcdir)/$(GCVERS)/include/gc_version.h
pkg_gc_include_HEADERS += $(top_srcdir)/$(GCVERS)/include/javaxfc.h
pkg_gc_include_HEADERS += $(top_srcdir)/$(GCVERS)/include/leak_detector.h

#---------------------------------------------------------------------

bin_PROGRAMS += hiha

hiha_LDFLAGS =
hiha_LDADD =
hiha_CFLAGS =
EXTRA_hiha_DEPENDENCIES =
hiha_SOURCES =

hiha_LDFLAGS += -export-dynamic

hiha_LDADD += libhiha.la
hiha_LDADD += $(GCVERS)/libhiha-gc.la
hiha_LDADD += lib/libgnu.la
hiha_LDADD += $(LTLIBICONV)
hiha_LDADD += $(LTLIBINTL)
hiha_LDADD += $(LTLIBUNISTRING)

hiha_CFLAGS += $(BASICCFLAGS)
hiha_CFLAGS += $(CFLAGS)

EXTRA_hiha_DEPENDENCIES += libhiha.la
EXTRA_hiha_DEPENDENCIES += lib/libgnu.la

hiha_SOURCES += src/hiha/hiha.c

#---------------------------------------------------------------------

lib_LTLIBRARIES += libhiha.la

libhiha_la_LIBADD =
libhiha_la_LDFLAGS =
libhiha_la_CFLAGS =
EXTRA_libhiha_la_DEPENDENCIES =
libhiha_la_SOURCES =

libhiha_la_LIBADD += lib/libgnu.la
#libhiha_la_LIBADD += $(GCVERS)/libhiha-gc.la

libhiha_la_LDFLAGS += $(LIBS)
libhiha_la_LDFLAGS += $(LTLIBICONV)
libhiha_la_LDFLAGS += $(LTLIBINTL)
libhiha_la_LDFLAGS += $(LTLIBUNISTRING)
libhiha_la_LDFLAGS += -lm -ldl
libhiha_la_LDFLAGS += -export-dynamic

libhiha_la_CFLAGS += $(BASICCFLAGS)
libhiha_la_CFLAGS += $(CFLAGS)

libhiha_HDRS =
libhiha_HDRS += include/libhiha/libhiha.h
libhiha_HDRS += include/libhiha/spinlock.h
libhiha_HDRS += include/libhiha/initialize_once.h
libhiha_HDRS += include/libhiha/string_t.h
libhiha_HDRS += include/libhiha/token_t.h
libhiha_HDRS += include/libhiha/pratt.h
libhiha_HDRS += include/libhiha/lexical.h
libhiha_HDRS += include/libhiha/load_plugin.h

pkg_libhiha_include_HEADERS += $(libhiha_HDRS)

libhiha_la_SOURCES += $(libhiha_HDRS)
libhiha_la_SOURCES += src/libhiha/spinlock.c
libhiha_la_SOURCES += src/libhiha/string_t.c
libhiha_la_SOURCES += src/libhiha/token_t.c
libhiha_la_SOURCES += src/libhiha/pratt.c
libhiha_la_SOURCES += src/libhiha/lexical.c
libhiha_la_SOURCES += src/libhiha/load_plugin.c

#---------------------------------------------------------------------
#
#pkg_plugins_lib_LTLIBRARIES += 10-basic-code-point-classification.la
#
#10_basic_code_point_classification_la_LIBADD =
#10_basic_code_point_classification_la_LDFLAGS =
#10_basic_code_point_classification_la_CFLAGS =
#EXTRA_10_basic_code_point_classification_la_DEPENDENCIES =
#10_basic_code_point_classification_la_SOURCES =
#
#10_basic_code_point_classification_la_LIBADD += libhiha.la
#10_basic_code_point_classification_la_LIBADD += lib/libgnu.la
#10_basic_code_point_classification_la_LIBADD += $(GCVERS)/libhiha-gc.la
#
#10_basic_code_point_classification_la_LDFLAGS += $(LIBS)
#10_basic_code_point_classification_la_LDFLAGS += $(LTLIBICONV)
#10_basic_code_point_classification_la_LDFLAGS += $(LTLIBINTL)
#10_basic_code_point_classification_la_LDFLAGS += $(LTLIBUNISTRING)
#10_basic_code_point_classification_la_LDFLAGS += -avoid-version -module
#10_basic_code_point_classification_la_LDFLAGS += -shared
#
#10_basic_code_point_classification_la_CFLAGS += $(BASICCFLAGS)
#10_basic_code_point_classification_la_CFLAGS += $(CFLAGS)
#
#10_basic_code_point_classification_la_SOURCES += \
#	src/plugins/10-basic-code-point-classification.c
#
#---------------------------------------------------------------------

.PHONY: indent
indent:
	$(INDENT) -l72 --no-tabs \
		-TFILE \
		-Tspinlock_t \
		-Tpratt_tables_t \
		-Tpratt_binding_power_t \
		-Tnud_handler_t \
		-Tled_handler_t \
		-Tstring_t \
		-Ttoken_t \
		-Ttext_location_t \
		-Ttoken_getter_t \
		-Ttoken_getter_from_source_file_t \
		-Ttoken_getter_from_serialized_tokens_t \
		-Tbuffered_token_getter_t \
		-Ttoken_putter_t \
		-Ttoken_putter_to_stream_serialized_t \
		-Thiha_plugin_t \
		-Thiha_plugin_tag_t \
		-Thiha_options_t \
		$(addprefix $(top_srcdir)/, \
			$(filter %.c %.h,$(libhiha_la_SOURCES))) \
		$(addprefix $(top_srcdir)/, \
			$(filter %.c %.h,$(hiha_SOURCES))) \
		$(addprefix $(top_srcdir)/, \
			$(filter %.c %.h,$(10_eof_la_SOURCES))) \
		$(addprefix $(top_srcdir)/, \
			$(filter %.c %.h,$(20_white_space_la_SOURCES))) \
		$(addprefix $(top_srcdir)/, \
			$(filter %.c %.h,$(30_decimal_integer_la_SOURCES)))

#---------------------------------------------------------------------

dist: $(DISTITEMS)

DISTITEMS =

$(srcdir)/README: $(srcdir)/README.adoc
	asciidoctor $(<) -o - \
		| elinks -dump 1 -no-references -no-numbering > $(@)
DISTITEMS += $(srcdir)/README

if HAVE_MERCURIAL
#
# The changelogging here is for a Mercurial repository. At the time of
# this writing I have not yet made public a Mercurial repository.
# However, the log will include the Git hashes of the changesets.
#
# Those who fork the public Git repository may prefer to design their
# own ChangeLog.
#
CHANGELOG_TEMPLATE = "{ifeq(bookmarks,'','','{bookmarks}\n')}{localdate(date, 'UTC')|date|isodate}\n{author}\ngit:       {gitnode}\nmercurial: {node}\nrevision:  {rev}\nparents:   {ifeq(p1.rev,'-1','','{p1.rev}')}{ifeq(p2.rev,'-1','',' {p2.rev}')}\n\n{indent('{desc|fill68}','  ')}\n\n\n"
$(srcdir)/ChangeLog: $(srcdir)/.hg
	$(MERCURIAL) log -G -T $(CHANGELOG_TEMPLATE) -C $(srcdir) \
	  > $(@).tmp && mv $(@){.tmp,}
DISTITEMS += $(srcdir)/ChangeLog
endif HAVE_MERCURIAL

#---------------------------------------------------------------------
# local variables:
# coding: utf-8
# tab-width: 4
# end:
