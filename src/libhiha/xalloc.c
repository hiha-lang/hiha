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

#include <config.h>
#include <string.h>
#include <gc/gc.h>
#include <xalloc.h>
#include <libhiha/xalloc.h>

#define _(msgid) HIHA_GETTEXT (msgid)

static void *
check_nonnull (void *p)
{
  if (p == NULL)
    xalloc_die ();
  return p;
}

HIHA_VISIBLE void *
xmalloc_atomic (size_t sz)
{
  return check_nonnull (GC_MALLOC_ATOMIC (sz));
}

HIHA_VISIBLE void *
xzalloc_atomic (size_t sz)
{
  void *p = xmalloc_atomic (sz);
  memset (p, 0, sz);
  return p;
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
