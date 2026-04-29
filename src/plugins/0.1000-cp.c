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
#include <libhiha/libhiha.h>

static void
code_point_handler (void *state, buffered_token_getter_t getter,
                    pratt_tables_t tables, token_t tok, void **lhs,
                    const char **error_message)
{
  if (*error_message == NULL)
    *lhs = (void *) tok;
}

HIHA_VISIBLE void
plugin_init (void)
{
  pratt_tables_t tables = lexical_pratt_tables ();
  pratt_nud_put (tables, string_t_CP (), &code_point_handler);
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
