# -*- autoconf -*-
#
# Copyright (C) 2013, 2026 Khaled Hosny and Barry Schwartz
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.

# serial 1

m4_define([__chem_gnu_compiler_flags],[{ :;
   if test x"${$2}" = xyes; then
      AC_LANG_PUSH([$1])
      CHEM_COMPILER_FLAGS([$3],[$4])
      m4_ifval([$5],[
         if test x"${with_gnu_ld}" = xyes; then
            CHEM_COMPILER_FLAGS([$3],[$5])
         fi
      ])
      AC_LANG_POP
   fi
}])

# CHEM_COMPILER_FLAGS(variable, -flag1 -flag2 ...)
# ------------------------------------------------
#
# Repeated calls to GNU Autoconf Archive macro AX_CHECK_COMPILE_FLAG.
#
AC_DEFUN([CHEM_COMPILER_FLAGS],[{ :;
   m4_foreach_w([__flag],[$2],
                [AX_CHECK_COMPILE_FLAG(__flag,[$1="[$]{$1} __flag"])])
}])

# CHEM_GNU_C_FLAGS(variable, -flag1 -flag2 ..., [-lflag1 -lflag2 ...])
# --------------------------------------------------------------------
#
# Call CHEM_COMPILER_FLAGS if the C compiler is GNU. If the linker is
# GNU ld, check the flags -lflag1, -lflag2, ..., as well.
#
AC_DEFUN([CHEM_GNU_C_FLAGS],
         [__chem_gnu_compiler_flags([C],[GCC],[$1],[$2],[$3])])

# CHEM_GNU_CXX_FLAGS(variable, -flag1 -flag2 ..., [-lflag1 -lflag2 ...])
# ----------------------------------------------------------------------
#
# Call CHEM_COMPILER_FLAGS if the C++ compiler is GNU. If the linker is
# GNU ld, check the flags -lflag1, -lflag2, ..., as well.
#
AC_DEFUN([CHEM_GNU_CXX_FLAGS],
         [__chem_gnu_compiler_flags([C++],[GXX],[$1],[$2],[$3])])

# CHEM_GNU_F77_FLAGS(variable, -flag1 -flag2 ..., [-lflag1 -lflag2 ...])
# ----------------------------------------------------------------------
#
# Call CHEM_COMPILER_FLAGS if the Fortran 77 compiler is GNU. If the
# linker is GNU ld, check the flags -lflag1, -lflag2, ..., as well.
#
AC_DEFUN([CHEM_GNU_F77_FLAGS],
         [__chem_gnu_compiler_flags([Fortran 77],[G77],[$1],[$2],[$3])])

# CHEM_GNU_FC_FLAGS(variable, -flag1 -flag2 ..., [-lflag1 -lflag2 ...])
# ---------------------------------------------------------------------
#
# Call CHEM_COMPILER_FLAGS if the Fortran compiler is GNU. If the
# linker is GNU ld, check the flags -lflag1, -lflag2, ..., as well.
#
AC_DEFUN([CHEM_GNU_FC_FLAGS],
         [__chem_gnu_compiler_flags([Fortran],[GFC],[$1],[$2],[$3])])
