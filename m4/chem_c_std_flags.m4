# -*- autoconf -*-
#
# Copyright (C) 2026 Barry Schwartz
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.

# serial 1

m4_define([_chem_c_std_flags],
[if true; then
  AC_LANG_PUSH([C])
  CHEM_COMPILER_FLAGS([$1],[$2])
  AC_LANG_POP
fi])

# CHEM_C23_FLAGS(variable)
# ------------------------
#
# Try to set variable to flag or flags for the best C23
# implementation. An implementation with extensions might be preferred
# over a strict implementation.
#
AC_DEFUN([CHEM_C23_FLAGS],
[AC_REQUIRE([AC_PROG_GREP])
 if true; then
  _chem_c_std_flags([$1],[-std=c2x -std=c23 -std=gnu2x -std=gnu23])
  echo ${$1} | ${GREP} -F -q std=gnu && \
       $1=`echo ${$1} | sed -e 's/-std=c2x//g' -e 's/-std=c23//g'`
  echo ${$1} | ${GREP} -F -q 23 && \
       $1=`echo ${$1} | sed -e 's/-std=c2x//g' -e 's/-std=gnu2x//g'`
fi])
