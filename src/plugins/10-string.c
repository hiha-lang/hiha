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

static void
scan_string (buffered_token_getter_t getter, token_t tok, token_t *lhs,
             const char **error_message)
{
  token_t t;
  getter->push_back_token (getter, tok, error_message);
  if (*error_message == NULL)
    scan_string_literal (getter, &t, NULL, error_message);
  *lhs = (*error_message == NULL) ? t : NULL;
}

nud_handler_t next_cp_handler;

static void
cp_handler (void *state, buffered_token_getter_t getter,
            token_putter_t putter, pratt_tables_t tables, token_t tok,
            token_t *lhs, const char **error_message)
{
  if (*error_message == NULL)
    {
      if (tok->token_value->n == 1 && tok->token_value->s[0] == '"')
        scan_string (getter, tok, lhs, error_message);
      else
        next_cp_handler (state, getter, putter, tables, tok, lhs,
                         error_message);
    }
}

HIHA_VISIBLE void
plugin_init (void)
{
  pratt_tables_t tables;

  acquire_pratt_tables_lock ();
  tables = get_pratt_tables_for_pass ("100-scan-string-literals");
  next_cp_handler = pratt_nud_get (tables, string_t_CP (), NULL);
  pratt_nud_put (tables, string_t_CP (), NULL, &cp_handler);
  set_pratt_tables_for_pass ("100-scan-string-literals", tables);
  release_pratt_tables_lock ();
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
