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

  ----------------------
  DECIMAL FLOATING POINT
  ----------------------

*/

#include <config.h>
#include <string.h>
#include <xalloc.h>
#include <error.h>
#include <exitfail.h>
#include <libhiha/libhiha.h>

// Change this if using gettext.
#define _(msgid) msgid

static bool
token_is_fraction_slash (token_t tok)
{
  /*

     The solidus /, DIVISION SLASH “∕” U+2215, or FRACTION SLASH “⁄”
     U+2044

     The fraction slash might be useful for pretty printing, depending
     on how one’s typesetting system works. Otherwise I would not
     recommend it, because numerals written with the fraction slash
     could get mistaken for Unicode predefined fractions, which are not
     treated by hiha as numerals.

   */

  return (string_t_cmp (tok->token_value, make_string_t ("/")) == 0
          || string_t_cmp (tok->token_value, make_string_t ("∕")) == 0
          || string_t_cmp (tok->token_value,
                           make_string_t ("⁄")) == 0);
}

static bool
token_is_i10 (token_t tok)
{
  return (string_t_cmp (tok->token_kind, make_string_t ("I10")) == 0);
}

nud_handler_t next_i10_handler;

static void
i10_handler (void *state, buffered_token_getter_t getter,
             pratt_tables_t tables, token_t tok, void **lhs,
             const char **error_message)
{
  bool done = (*error_message != NULL);
  if (!done)
    {
      token_t t;
      getter->look_at_token (getter, 0, &t, error_message);
      done = (*error_message != NULL);
      if (!done && token_is_fraction_slash (t))
        {
          token_t u;
          getter->look_at_token (getter, 1, &u, error_message);
          done = (*error_message != NULL);
          if (!done && token_is_i10 (u))
            {
              string_t str =
                concat_string_t (tok->token_value, t->token_value,
                                 u->token_value, NULL);
              *lhs =
                (void *) make_token_t (make_string_t ("R10"), str,
                                       tok->loc);
              (void) getter->get_token (getter, &u, error_message);
              if (*error_message == NULL)
                (void) getter->get_token (getter, &u, error_message);
              done = true;
            }
        }
    }
  if (!done)
    next_i10_handler (state, getter, tables, tok, lhs, error_message);
}

HIHA_VISIBLE void
plugin_init (void)
{
  pratt_tables_t tables = lexical_pratt_tables ();
  next_i10_handler = pratt_nud_get (tables, make_string_t ("I10"));
  pratt_nud_put (tables, make_string_t ("I10"), &i10_handler);
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
