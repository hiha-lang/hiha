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

AC_PREREQ([2.72])
AC_INIT([hiha],[0.0.0])

AC_CONFIG_SRCDIR([src/hiha/hiha.c])

AC_CONFIG_HEADERS([config.h])
#AC_CONFIG_TESTDIR([tests])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AC_CANONICAL_BUILD                 dnl On what platform are we compiling?
AC_CANONICAL_HOST                  dnl For what platform are we compiling?
AC_USE_SYSTEM_EXTENSIONS           dnl Define such macros as _GNU_SOURCE.

AM_INIT_AUTOMAKE([tar-pax no-dist-gzip dist-xz])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE

AC_PROG_CC
gl_EARLY
AC_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P
AC_PROG_AWK
AC_PROG_SED
AC_PROG_GREP
AC_PROG_FGREP
AC_PROG_EGREP

CHEM_GNU_C_FLAGS([__c_compiler_flags],[-std=gnu23])
CFLAGS="${CFLAGS}${CFLAGS+ }${__c_compiler_flags}"

CHEM_PATH_PROGS_CACHED_AND_PRECIOUS([INDENT],
  [GNU indent],[indent],
  [
    if LC_ALL=C LANG=C ${ac_path_INDENT} --version 2>&1 | \
      LC_ALL=C LANG=C ${GREP} -e '^GNU indent' \
               2> /dev/null > /dev/null; then
      ac_cv_path_INDENT="${ac_path_INDENT}"
      ac_path_INDENT_found=:
    fi
  ])

# CHEM_PROG_GNU_M4
# if test -z "${GNU_M4}"; then
#    AC_MSG_ERROR([GNU m4 not found. Perhaps set GNU_M4 to OpenBSD m4.])
# fi

# CHEM_UNIQUE_ID([UNIQUE_ID])

gl_INIT

AC_CHECK_TYPE([_Decimal32],[],[
  # We use decimal numbers for binding powers. There is no point
  # catering to clang, which I make a point of keeping off my system
  # to avoid having it fill up with ever-changing, bloated LLVMs. I
  # would sooner implement my own BCD fixed point, given all we need
  # is binary comparisons—and you can confuse the kiddies by
  # reëncoding a source code string into EBCDIC to get the BCD.
  # Floating point might be slightly better, however, because what
  # constitutes a huge value is more obvious.
  AC_MSG_ERROR([_Decimal32 type not found. If you are using clang, consider GCC instead.])
])

# You can use ‘@GNU@’ to make GNU Make constructs unbothersome to
# Automake. (Often you can use ‘$(eval ...)’ instead, but @GNU@ is
# more straightforward.)
AC_SUBST([GNU],[])

CHEM_REQUIRE_GNU_MAKE_IN_PATH
CHEM_CONFIG_MAKEFILES
CHEM_CONFIG_MAKEFILES([lib/Makefile],[lib/GNUmakefile])
#CHEM_CONFIG_MAKEFILES([tests/Makefile],[tests/GNUmakefile])

AC_OUTPUT

# local variables:
# coding: utf-8
# end:
