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

#define _(msgid) HIHA_GETTEXT (msgid)

static bool
token_is_white_space (token_t tok)
{
  return (tok->token_value->n == 1
          && (uc_is_property
              (tok->token_value->s[0], UC_PROPERTY_WHITE_SPACE)));
}

static bool
token_is_percent_sign (token_t tok)
{
  return (tok->token_value->n == 1 && tok->token_value->s[0] == '%');
}

static void
scan_comment (buffered_token_getter_t getter, token_t tok, token_t *lhs,
              const char **error_message)
{
  string_t str = tok->token_value;
  token_t t;
  do
    {
      getter->get_token (getter, &t, error_message);
      if (*error_message == NULL)
        str = concat_string_t (str, t->token_value, NULL);
    }
  while (*error_message == NULL
         && t->token_value->n == 1
         && 0 != string_t_cmp (t->token_value, make_string_t ("\n")));
  if (*error_message == NULL)
    *lhs = make_token_t (make_string_t ("CO"), str, tok->loc);
}

nud_handler_t next_cp_handler;

static void
cp_handler (void *state, buffered_token_getter_t getter,
            pratt_tables_t tables, token_t tok, token_t *lhs,
            const char **error_message)
{
  if (*error_message == NULL)
    {
      if (token_is_white_space (tok))
        *lhs =
          make_token_t (make_string_t ("SP"), tok->token_value,
                        tok->loc);
      else if (token_is_percent_sign (tok))
        scan_comment (getter, tok, lhs, error_message);
      else
        next_cp_handler (state, getter, tables, tok, lhs,
                         error_message);
    }
}

HIHA_VISIBLE void
plugin_init (void)
{
  pratt_tables_t tables;

  acquire_pratt_tables_lock ();

  tables =
    get_pratt_tables_for_pass ("100-scan-white-space-and-comments");
  next_cp_handler = pratt_nud_get (tables, string_t_CP ());
  pratt_nud_put (tables, string_t_CP (), &cp_handler);
  set_pratt_tables_for_pass ("100-scan-white-space-and-comments",
                             tables);

  release_pratt_tables_lock ();
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
