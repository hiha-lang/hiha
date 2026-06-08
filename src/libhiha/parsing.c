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
#include <math.h>
#include <float.h>
#include <libhiha/persistent_hash_map.h>
#include <libhiha/parsing.h>

#define _(msgid) HIHA_GETTEXT (msgid)

HIHA_VISIBLE parsing_state_t
make_parsing_state_t (void)
{
  parsing_state_t p = XMALLOC (struct parsing_state);
  return p;
};


/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
