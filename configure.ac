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

LT_PREREQ([2.5.4])
LT_INIT([pic-only,dlopen])

AC_ARG_ENABLE([fixed-point-hash],
  [AS_HELP_STRING([--enable-fixed-point-hash=blake3,sha256 [blake3]],
      [hash algorithm for token-stream fixed-point detection])],
  [],
  [enable_fixed_point_hash=blake3])

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

CHEM_GNU_C_FLAGS([__c_compiler_flags],[
  -std=gnu23
  -fvisibility=hidden
])
CFLAGS="${CFLAGS}${CFLAGS+ }${__c_compiler_flags}"

CHEM_GNU_C_FLAGS([__visible_flags],[
  -fPIC
  -fvisibility=default
])
AC_SUBST([VISIBLE_CFLAGS],["${__visible_flags}"])

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

if test "${enable_fixed_point_hash}" = blake3; then
  PKG_CHECK_MODULES([LIBBLAKE3],[libblake3])
  CFLAGS="${CFLAGS}${CFLAGS+ }${LIBBLAKE3_CFLAGS}"
  LIBS="${LIBS}${LIBS+ }${LIBBLAKE3_LIBS}"
  AC_DEFINE([USE_LIBBLAKE3],[1],[Define to 1 if you want to use libblake3.])
elif test "${enable_fixed_point_hash}" = sha256; then
  AC_DEFINE([USE_SHA256],[1],[Define to 1 if you want to use SHA256.])
else
  AC_MSG_ERROR([unknown hash algorithm “${enable_fixed_point_hash}”])
fi

if test x${host_cpu} = xi386 || test x${host_cpu} = xx86_64; then
  AC_CACHE_CHECK([for __builtin_ia32_pause],
                 [hiha_cv_func_have___builtin_ia32_pause],
                 [hiha_cv_func_have___builtin_ia32_pause=no
                  AC_COMPILE_IFELSE([
                    AC_LANG_PROGRAM([[]],[[__builtin_ia32_pause();]])
                  ],
                  [hiha_cv_func_have___builtin_ia32_pause=yes])])
  if test x"${hiha_cv_func_have___builtin_ia32_pause}" = xyes; then
    AC_DEFINE([HAVE___BUILTIN_IA32_PAUSE],[1],
              [Define if __builtin_ia32_pause exists.])
  fi
  AC_SUBST([have___builtin_ia32_pause],
           ["${hiha_cv_func_have___builtin_ia32_pause}"])
fi

# You can use ‘@GNU@’ to make GNU Make constructs unbothersome to
# Automake. (Often you can use ‘$(eval ...)’ instead, but @GNU@ is
# more straightforward.)
AC_SUBST([GNU],[])

CHEM_REQUIRE_GNU_MAKE_IN_PATH
CHEM_CONFIG_MAKEFILES
CHEM_CONFIG_MAKEFILES([lib/Makefile],[lib/GNUmakefile])
#CHEM_CONFIG_MAKEFILES([tests/Makefile],[tests/GNUmakefile])

AC_DEFINE([PCRE2_CODE_UNIT_WIDTH],[32],[Code unit width for PCRE2.])

AC_OUTPUT

# local variables:
# coding: utf-8
# end:
