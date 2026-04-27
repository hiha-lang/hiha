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

// Change this if using gettext.
#define _(msgid) msgid

#define VISIBLE [[gnu::visibility ("default")]]

static void
eof_handler (void *state, pratt_tables_t tables,
             token_t tok, void **lhs, const char **error_message)
{
  *error_message = NULL;
  *lhs = (void *) tok;
}

static void
check_code_point_token (token_t tok)
{
  if (tok->token_value->n != 1)
    error (exit_failure, 0,
           "CP token with a value of length other than 1: “%s”",
           make_str_nul (tok->token_value));
}

static inline bool
is_ascii_digit (uint32_t cp)
{
  return ('0' <= cp && cp <= '9');
}

static void
code_point_handler (void *state, pratt_tables_t tables,
                    token_t tok, void **lhs, const char **error_message)
{
  check_code_point_token (tok);
  *error_message = NULL;
  uint32_t cp = tok->token_value->s[0];
  if (uc_is_property (cp, UC_PROPERTY_WHITE_SPACE))
    *lhs =
      (void *) make_token_t (make_string_t ("SP"), tok->token_value,
                             tok->loc);
  else if (uc_is_property (cp, UC_PROPERTY_ID_START))
    *lhs =
      (void *) make_token_t (make_string_t ("IDS"), tok->token_value,
                             tok->loc);
  else if (uc_is_property (cp, UC_PROPERTY_ID_CONTINUE))
    *lhs =
      (void *) make_token_t (make_string_t ("IDC"), tok->token_value,
                             tok->loc);
  else if (uc_is_general_category (cp, UC_CATEGORY_Sm))
    // Math symbols.
    *lhs =
      (void *) make_token_t (make_string_t ("Sm"), tok->token_value,
                             tok->loc);
  /*
  else if (uc_is_property (cp, UC_PROPERTY_QUOTATION_MARK))
    *lhs =
      (void *) make_token_t (make_string_t ("QU"), tok->token_value,
                             tok->loc);
  */
  else
    *lhs = (void *) tok;
}

VISIBLE void
plugin_init (void)
{
  pratt_tables_t tables = lexical_pratt_tables ();
  pratt_nud_put (tables, string_t_EOF (), &eof_handler);
  pratt_nud_put (tables, string_t_CP (), &code_point_handler);
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
