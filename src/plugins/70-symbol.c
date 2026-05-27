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
#include <xalloc.h>
#include <libhiha/libhiha.h>

#define _(msgid) HIHA_GETTEXT (msgid)

static bool
is_symbol_part (token_t tok)
{
  /* Any symbol with the mathematics property, or the hyphen (which
     often is used as an ugly minus sign, and which hiha is not using
     as a hyphen). */
  return
    (tok->token_value->n == 1
     && ((uc_is_property (tok->token_value->s[0], UC_PROPERTY_MATH)
          && uc_is_general_category (tok->token_value->s[0],
                                     UC_CATEGORY_S))
         || tok->token_value->s[0] == '-'));
}

static void
scan_symbol (void *state, buffered_token_getter_t getter,
             pratt_tables_t tables, token_t tok, void **lhs,
             const char **error_message)
{
  string_t tokval = tok->token_value;
  token_t t;
  getter->look_at_token (getter, 0, &t, error_message);
  while (*error_message == NULL && is_symbol_part (t))
    {
      getter->get_token (getter, &t, error_message);
      if (*error_message == NULL)
        {
          tokval = concat_string_t (tokval, t->token_value, NULL);
          getter->look_at_token (getter, 0, &t, error_message);
        }
    }
  if (*error_message == NULL)
    *lhs =
      (void *) make_token_t (make_string_t ("SY"), tokval, tok->loc);
}

nud_handler_t next_cp_handler;

static void
code_point_handler (void *state, buffered_token_getter_t getter,
                    pratt_tables_t tables, token_t tok, void **lhs,
                    const char **error_message)
{
  if (*error_message == NULL)
    {
      if (is_symbol_part (tok))
        scan_symbol (state, getter, tables, tok, lhs, error_message);
      else
        next_cp_handler (state, getter, tables, tok, lhs,
                         error_message);
    }
}

HIHA_VISIBLE void
plugin_init (void)
{
  pratt_tables_t tables = lexical_pratt_tables ();
  next_cp_handler = pratt_nud_get (tables, string_t_CP ());
  pratt_nud_put (tables, string_t_CP (), &code_point_handler);
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
