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
#include <libhiha/token_t.h>

// Change this if using gettext.
#define _(msgid) msgid

void
token_t_free (token_t tok)
{
  if (tok != NULL)
    {
      string_t_free (tok->token_kind);
      string_t_free (tok->token_value);
    }
  free (tok);
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
