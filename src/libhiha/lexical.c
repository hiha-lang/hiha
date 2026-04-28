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

/*

  Lexical analysis via Pratt parser
  ---------------------------------

  Use the Pratt parser to convert token streams to token streams.
  Continue until a fixed point is reached.

*/

#include <config.h>
#include <math.h>
#include <error.h>
#include <exitfail.h>
#include <libhiha/lexical.h>
#include <libhiha/initialize_once.h>

// Change this if using gettext.
#define _(msgid) msgid

#define VISIBLE [[gnu::visibility ("default")]]

static initialize_once_t _lexical_tables_init1t =
  INITIALIZE_ONCE_T_INIT;
static pratt_tables_t _lexical_tables = NULL;

static void
_initialize_lexical_tables (void)
{
  _lexical_tables = make_pratt_tables_t ();
}

VISIBLE pratt_tables_t
lexical_pratt_tables (void)
{
  INITIALIZE_ONCE (_lexical_tables_init1t, _initialize_lexical_tables);
  return _lexical_tables;
}

static token_t
lhs_to_token_t (void *lhs, const char *error_message)
{
  return (error_message == NULL) ? (token_t) lhs : NULL;
}

VISIBLE void
scan_tokens (void *state, buffered_token_getter_t getter,
             token_putter_t putter, const char **error_message)
{
  token_t tok;
  void *lhs = NULL;
  pratt_tables_t tables = lexical_pratt_tables ();

  *error_message = NULL;

  pratt_parse (state, getter, tables, -HUGE_VAL, &lhs, error_message);
  tok = lhs_to_token_t (lhs, *error_message);
  while (*error_message == NULL
         && string_t_cmp (tok->token_kind, string_t_EOF ()))
    {
      putter->put_token (putter, tok, error_message);
      if (*error_message == NULL)
        {
          pratt_parse (state, getter, tables, -HUGE_VAL, &lhs,
                       error_message);
          tok = lhs_to_token_t (lhs, *error_message);
        }
    }
  if (*error_message == NULL)
    putter->put_token (putter, tok, error_message);
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
