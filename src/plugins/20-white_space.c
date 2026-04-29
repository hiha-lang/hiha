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
#include <error.h>
#include <exitfail.h>
#include <unictype.h>
#include <libhiha/libhiha.h>

// Change this if using gettext.
#define _(msgid) msgid

static void
check_code_point_token (token_t tok)
{
  if (tok->token_value->n != 1)
    error (exit_failure, 0,
           "CP token with a value of length other than 1: “%s”",
           make_str_nul (tok->token_value));
}

nud_handler_t next_handler;

static void
code_point_handler (void *state, buffered_token_getter_t getter,
                    pratt_tables_t tables, token_t tok, void **lhs,
                    const char **error_message)
{
  if (*error_message == NULL)
    {
      check_code_point_token (tok);
      if (uc_is_property
          (tok->token_value->s[0], UC_PROPERTY_WHITE_SPACE))
        *lhs =
          (void *) make_token_t (make_string_t ("SP"), tok->token_value,
                                 tok->loc);
      else
        next_handler (state, getter, tables, tok, lhs, error_message);
    }
}

HIHA_VISIBLE void
plugin_init (void)
{
  pratt_tables_t tables = lexical_pratt_tables ();
  next_handler = pratt_nud_get (tables, string_t_CP ());
  pratt_nud_put (tables, string_t_CP (), &code_point_handler);
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
