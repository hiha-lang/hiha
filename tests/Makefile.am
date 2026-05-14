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

AUTOMAKE_OPTIONS =
AUTOMAKE_OPTIONS += -Wno-portability

@GNU@ include $(top_srcdir)/silent-rules.mk

AM_LIBRS =
LDADD =
EXTRA_DIST =
EXTRA_PROGRAMS =
BUILT_SOURCES =
CLEANFILES =
DISTCLEANFILES =

check_PROGRAMS =
check_SCRIPTS =

DISTCLEANFILES += Makefile
DISTCLEANFILES += GNUmakefile
DISTCLEANFILES += atconfig

$(srcdir)/package.m4: $(top_srcdir)/configure.ac $(srcdir)/Makefile.am
	$(AWK) \
	  'BEGIN{printf "# Signature of the current package.\n"; \
	         printf "m4_define([AT_PACKAGE_NAME],"; \
	         printf "[$(PACKAGE_NAME)])\n"; \
	         printf "m4_define([AT_PACKAGE_TARNAME],"; \
	         printf "[$(PACKAGE_TARNAME)])\n"; \
	         printf "m4_define([AT_PACKAGE_VERSION],"; \
	         printf "[$(PACKAGE_VERSION)])\n"; \
	         printf "m4_define([AT_PACKAGE_STRING],"; \
	         printf "[$(PACKAGE_STRING)])\n"; \
	         printf "m4_define([AT_PACKAGE_BUGREPORT],"; \
	         printf "[$(PACKAGE_BUGREPORT)])\n"; \
	         printf "m4_define([AT_PACKAGE_URL],"; \
	         printf "[$(PACKAGE_URL)])\n"}' \
	  > $(@).tmp && mv $(@){.tmp,}

CLEANFILES += *.tmp
CLEANFILES += tmp.*

EXTRA_DIST += testsuite.at
EXTRA_DIST += $(srcdir)/package.m4
EXTRA_DIST += $(TESTSUITE)
EXTRA_DIST += atlocal.in

@GNU@ define jobs-flag
@GNU@ $(filter -j%,$(MAKEFLAGS))
@GNU@ endef

@GNU@ define color-flag
@GNU@ $(shell \
@GNU@   if test "${enable_color}" = no; then \
@GNU@     printf '%s' "--color=never"; \
@GNU@   else \
@GNU@     printf '%s' "--color=auto"; \
@GNU@   fi \
@GNU@ )
@GNU@ endef

BUNCHOFFLAGS = $(color-flag) $(jobs-flag) $(TESTSUITEFLAGS)

check-local: atconfig atlocal $(TESTSUITE)
	$(SHELL) '$(TESTSUITE)' $(BUNCHOFFLAGS)

installcheck-local: atconfig atlocal $(TESTSUITE)
	@if false; then \
	  $(SHELL) '$(TESTSUITE)' AUTOTEST_PATH='$(bindir)' $(BUNCHOFFLAGS); \
	fi			# installcheck is disabled.

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean

AUTOM4TE = $(SHELL) $(top_srcdir)/build-aux/missing --run autom4te
AUTOTEST = $(AUTOM4TE) --language=autotest

TESTSUITE = $(srcdir)/testsuite

TESTSUITE_DEPS =

TESTSUITE_DEPS += $(srcdir)/atlocal.in
TESTSUITE_DEPS += $(srcdir)/package.m4
TESTSUITE_DEPS += $(srcdir)/testsuite.at

$(TESTSUITE): $(TESTSUITE_DEPS)
	$(AUTOTEST) -I '$(srcdir)' -o $(@).tmp $(srcdir)/testsuite.at \
	    && mv $(@){.tmp,}

AM_LIBRS += $(CLOCK_TIME_LIB)
AM_LIBRS += $(GETRANDOM_LIB)
AM_LIBRS += $(HARD_LOCALE_LIB)
AM_LIBRS += $(LIBPTHREAD)
AM_LIBRS += $(LIBTHREAD)
AM_LIBRS += $(LIBC32CONV)
AM_LIBRS += $(LIBICONV)
AM_LIBRS += $(LIBINTL)
AM_LIBRS += $(LIBUNISTRING)
AM_LIBRS += $(MBRTOWC_LIB)
AM_LIBRS += $(PTHREAD_ONCE_LIB)
AM_LIBRS += $(SETLOCAL_NULL_LIB)

LDADD += $(top_builddir)/libhiha.la
LDADD += $(top_builddir)/$(GCVERS)/libhiha-gc.la
LDADD += $(top_builddir)/lib/libgnu.la
LDADD += $(AM_LIBRS)

BASICCFLAGS =
BASICCFLAGS += -I $(abs_top_builddir)
BASICCFLAGS += -I $(abs_top_builddir)/include
BASICCFLAGS += -I $(abs_top_builddir)/lib
BASICCFLAGS += -I $(abs_top_srcdir)
BASICCFLAGS += -I $(abs_top_srcdir)/include
BASICCFLAGS += -I $(abs_top_srcdir)/lib
BASICCFLAGS += -I $(abs_top_srcdir)/$(GCVERS)/include

check_PROGRAMS += check_spookyhash
check_spookyhash_CFLAGS =
check_spookyhash_CFLAGS += $(BASICCFLAGS)
check_spookyhash_CFLAGS += $(CFLAGS)

# The SpookyHash reference implementation is written for i386/x86-64,
# and so should not be run on other architectures unless you know what
# you are doing.
$(srcdir)/spookyhash-reference.txt: SpookyV2-reference
	-rm -f $(@)
	touch $(@)
	./SpookyV2-reference 1 2 10000 >> $(@)
	./SpookyV2-reference 3 4 10000 >> $(@)

EXTRA_PROGRAMS += SpookyV2-reference
CLEANFILES += SpookyV2-reference
SpookyV2_reference_SOURCES =
SpookyV2_reference_CXXFLAGS =
SpookyV2_reference_CXXFLAGS += $(BASICCFLAGS)
SpookyV2_reference_CXXFLAGS += $(VISIBLE_CFLAGS)
SpookyV2_reference_LDADD =
SpookyV2_reference_LDADD += $(top_builddir)/lib/libgnu.la
SpookyV2_reference_LDADD += $(AM_LIBRS)
SpookyV2_reference_SOURCES += SpookyV2-reference.cpp
SpookyV2_reference_SOURCES += SpookyV2.cpp
SpookyV2_reference_SOURCES += SpookyV2.h

if ALLOW_NESTED_C_FUNCTIONS
check_PROGRAMS += check_persistent_avl
check_persistent_avl_CFLAGS =
check_persistent_avl_LDFLAGS =
check_persistent_avl_CFLAGS += $(BASICCFLAGS)
check_persistent_avl_CFLAGS += $(CFLAGS)
check_persistent_avl_LDFLAGS += $(AM_LDFLAGS)
check_persistent_avl_LDFLAGS += $(EXECSTACK_FLAGS)
endif ALLOW_NESTED_C_FUNCTIONS

if ALLOW_NESTED_C_FUNCTIONS
check_PROGRAMS += check_persistent_integer_trie
check_persistent_integer_trie_CFLAGS =
check_persistent_integer_trie_LDFLAGS =
check_persistent_integer_trie_CFLAGS += $(BASICCFLAGS)
check_persistent_integer_trie_CFLAGS += $(CFLAGS)
check_persistent_integer_trie_LDFLAGS += $(AM_LDFLAGS)
check_persistent_integer_trie_LDFLAGS += $(EXECSTACK_FLAGS)
endif ALLOW_NESTED_C_FUNCTIONS
