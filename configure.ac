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

CHEM_PATH_PROGS_CACHED_AND_PRECIOUS([MERCURIAL],
  [Mercurial Distributed SCM],[hg],
  [
    if LC_ALL=C LANG=C ${ac_path_MERCURIAL} --version 2>&1 | \
      LC_ALL=C LANG=C ${GREP} -e '^Mercurial Distributed SCM' \
               2> /dev/null > /dev/null; then
      ac_cv_path_MERCURIAL="${ac_path_MERCURIAL}"
      ac_path_MERCURIAL_found=:
    fi
  ])
AM_CONDITIONAL([HAVE_MERCURIAL],
        [test "${chem_cv_path_MERCURIAL_found}" = yes])

# CHEM_PROG_GNU_M4
# if test -z "${GNU_M4}"; then
#    AC_MSG_ERROR([GNU m4 not found. Perhaps set GNU_M4 to OpenBSD m4.])
# fi

# CHEM_UNIQUE_ID([UNIQUE_ID])

gl_INIT

host_is_x86=no
if test x${host_cpu} = xi386; then
  host_is_x86=yes
elif test x${host_cpu} = xx86_64; then
  host_is_x86=yes
elif test x${host_cpu} = xx86_64-x32; then
  host_is_x86=yes
fi

if test x${host_is_x86} = xyes; then
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

if test x${host_is_x86} = xyes; then
  AC_CACHE_CHECK([for _mm_pause],
                 [hiha_cv_func_have__mm_pause],
                 [hiha_cv_func_have__mm_pause=no
                  AC_COMPILE_IFELSE([
                    AC_LANG_PROGRAM([[#include <immintrin.h>]],
                                    [[_mm_pause();]])
                  ],
                  [hiha_cv_func_have__mm_pause=yes])])
  if test x"${hiha_cv_func_have__mm_pause}" = xyes; then
    AC_DEFINE([HAVE__MM_PAUSE],[1],
              [Define if _mm_pause exists.])
  fi
  AC_SUBST([have__mm_pause],
           ["${hiha_cv_func_have__mm_pause}"])
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

# We compile our own copy of Boehm GC, with
# enable-redirect-malloc=yes, which we need. (Below we have
# enable-threads=no only due to a “poor performance” warning from the
# package, for some combination of features.) Our library will be
# called libhiha-gc. There is no great reason not to include the
# corresponding “cord” library, so it will be called libhiha-cord.
GCVERS=gc-8.2.12
AC_SUBST([GCVERS])
AC_CONFIG_COMMANDS([configure-gc],
  [
    gcdir="${ac_abs_top_builddir}/${GCVERS}"
    mkdir -p "${gcdir}"
    (
      _cflags=`printf "%s" "${CFLAGS}" | ${SED} 's/fvisibility=hidden/fvisibility=default/g'`
      cd "${gcdir}"
      "${ac_abs_top_srcdir}/${GCVERS}"/configure \
        --prefix="${prefix}" \
        --bindir="${bindir}" \
        --sbindir="${sbindir}" \
        --libexecdir="${libexecdir}" \
        --sysconfdir="${sysconfdir}" \
        --sharedstatedir="${sharedstatedir}" \
        --localstatedir="${localstatedir}" \
        --runstatedir="${runstatedir}" \
        --libdir="${libdir}" \
        --includedir="${includedir}" \
        --oldincludedir="${oldincludedir}" \
        --datarootdir="${datarootdir}" \
        --datadir="${datadir}" \
        --infodir="${infodir}" \
        --localedir="${localedir}" \
        --mandir="${mandir}" \
        --docdir="${docdir}" \
        --htmldir="${htmldir}" \
        --dvidir="${dvidir}" \
        --pdfdir="${pdfdir}" \
        --psdir="${psdir}" \
        --enable-redirect-malloc=yes \
        --enable-silent-rules="${enable_silent_rules}" \
        --enable-maintainer-mode="${enable_maintainer_mode}" \
        --enable-shared=yes \
        --enable-static=yes \
        --enable-threads=no \
        CFLAGS="${_cflags}"
    )
    ${SED} -e 's/libgc/libhiha-gc/g' \
           -e 's/libcord/libhiha-cord/g' \
           "${gcdir}"/Makefile > "${gcdir}"/Makefile.tmp
    mv "${gcdir}"/Makefile{.tmp,}
  ],
  [
    SED="${SED}"
    CFLAGS="${CFLAGS}"
    GCVERS="${GCVERS}"
    ac_abs_top_srcdir="${ac_abs_top_srcdir}"
    ac_abs_top_builddir="${ac_abs_top_builddir}"
    enable_silent_rules="${enable_silent_rules}"
    enable_maintainer_mode="${enable_maintainer_mode}"
    prefix="${prefix}"
    bindir="${bindir}"
    sbindir="${sbindir}"
    libexecdir="${libexecdir}"
    sysconfdir="${sysconfdir}"
    sharedstatedir="${sharedstatedir}"
    localstatedir="${localstatedir}"
    runstatedir="${runstatedir}"
    libdir="${libdir}"
    includedir="${includedir}"
    oldincludedir="${oldincludedir}"
    datarootdir="${datarootdir}"
    datadir="${datadir}"
    infodir="${infodir}"
    localedir="${localedir}"
    mandir="${mandir}"
    docdir="${docdir}"
    htmldir="${htmldir}"
    dvidir="${dvidir}"
    pdfdir="${pdfdir}"
    psdir="${psdir}"
  ])

AC_DEFINE([HIHA_VISIBLE],[[[[gnu::visibility ("default")]]]],
  [Define as a C attribute for making a symbol visible.])
AC_DEFINE([HIHA_PURE],[[[[gnu::pure]]]],
  [Define as a C attribute for marking a function pure.])

AH_BOTTOM([
/* A workaround for a bug in Gnulib’s stdlib.h (2026 April 19). */
#define _GL_FUNCDECL_SYS_NAME(func) (func)
])

AC_OUTPUT

# local variables:
# coding: utf-8
# end:
