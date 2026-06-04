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

#define _(msgid) HIHA_GETTEXT (msgid)

static string_t str_SP;
static string_t str_FSEP;
static string_t str_CO;

static bool
token_is_space (token_t tok)
{
  return (string_t_cmp (tok->token_kind, str_SP) == 0
          || string_t_cmp (tok->token_kind, str_FSEP) == 0
          || string_t_cmp (tok->token_kind, str_CO) == 0);
}

nud_handler_t next_handler;

static void
handler (void *state, buffered_token_getter_t getter,
         pratt_tables_t tables, token_t tok,
         token_t *lhs, const char **error_message)
{
  /* Consume any run of spaces. */
  token_t t = tok;
  while (*error_message == NULL && token_is_space (t))
    getter->get_token (getter, &t, error_message);
  next_handler (state, getter, tables, t, lhs, error_message);
}

HIHA_VISIBLE void
plugin_init (void)
{
  str_SP = make_string_t ("SP");
  str_FSEP = make_string_t ("FSEP");
  str_CO = make_string_t ("CO");

  pratt_tables_t tables;

  acquire_pratt_tables_lock ();

  tables = get_pratt_tables_for_pass ("1500-remove-spaces");
  next_handler = pratt_nud_get_default (tables);
  pratt_nud_put_default (tables, &handler);
  set_pratt_tables_for_pass ("1500-remove-spaces", tables);

  release_pratt_tables_lock ();
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
