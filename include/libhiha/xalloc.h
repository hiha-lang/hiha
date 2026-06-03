/*

  The hiha programming language

  Copyright © 2026 Barry Schwartz

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#ifndef __LIBHAHA__XALLOC_H__INCLUDED__
#define __LIBHAHA__XALLOC_H__INCLUDED__

#include <stdlib.h>
#include <stddef.h>

#define HIHA_XALLOC_ATTR_MALLOC [[gnu::malloc (free, 1)]]
#define HIHA_XALLOC_ATTR_RETURNS_NONNULL [[gnu::returns_nonnull]]
#define HIHA_XALLOC_ATTR_ALLOC_SIZE(args) [[gnu::alloc_size args]]

HIHA_XALLOC_ATTR_MALLOC HIHA_XALLOC_ATTR_RETURNS_NONNULL
HIHA_XALLOC_ATTR_ALLOC_SIZE ((1))
     void *xmalloc_atomic (size_t s);

     HIHA_XALLOC_ATTR_MALLOC HIHA_XALLOC_ATTR_RETURNS_NONNULL
       HIHA_XALLOC_ATTR_ALLOC_SIZE ((1))
     void *xzalloc_atomic (size_t s);

#define XMALLOC_ATOMIC(t) ((t *) (xmalloc_atomic (sizeof (t))))
#define XNMALLOC_ATOMIC(n, t) (xmalloc_atomic ((n) * sizeof (t)))
#define XZALLOC_ATOMIC(t) ((t *) (xzalloc_atomic (sizeof (t))))
#define XCALLOC_ATOMIC(n, t) (xzalloc_atomic ((n) * sizeof (t)))

#endif /* __LIBHAHA__XALLOC_H__INCLUDED__ */

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
