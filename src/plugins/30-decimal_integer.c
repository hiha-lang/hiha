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

  ----------------
  DECIMAL INTEGERS
  ----------------

  Strings of ASCII digits 0123456789, with a single connector
  punctuation (Unicode Category Pc), such as an underscore _
  character, allowed between digits.

  
*/

#include <config.h>
#include <error.h>
#include <exitfail.h>
#include <unictype.h>
#include <xalloc.h>
#include <libhiha/libhiha.h>

#define _(msgid) HIHA_GETTEXT (msgid)

static bool
is_ascii_digit (uint32_t cp)
{
  return ('0' <= cp && cp <= '9');
}

static bool
token_is_ascii_digit (token_t tok)
{
  return (tok->token_value->n == 1
          && is_ascii_digit (tok->token_value->s[0]));
}

static bool
token_is_connecting_punctuation (token_t tok)
{
  return (tok->token_value->n == 1
          && uc_is_general_category (tok->token_value->s[0],
                                     UC_CATEGORY_Pc));
}

static bool
token_is_i10 (token_t tok)
{
  return (string_t_cmp (tok->token_kind, make_string_t ("I10")) == 0);
}

static void
consume_tokens (buffered_token_getter_t getter, size_t num_to_consume,
                const char **error_message)
{
  token_t t;
  size_t i = 0;
  while (*error_message == NULL && i != num_to_consume)
    {
      getter->get_token (getter, &t, error_message);
      i += 1;
    }
}

static void
get_next_digit (buffered_token_getter_t getter, uint32_t *separator,
                uint32_t *digit, const char **error_message)
{
  *separator = 0;
  *digit = 0;
  size_t num_to_consume = 0;

  token_t t;
  getter->look_at_token (getter, 0, &t, error_message);
  if (*error_message == NULL)
    {
      if (token_is_ascii_digit (t))
        {
          *digit = t->token_value->s[0];
          num_to_consume = 1;
        }
      else if (token_is_connecting_punctuation (t))
        {
          token_t u;
          getter->look_at_token (getter, 1, &u, error_message);
          if (*error_message == NULL && token_is_ascii_digit (u))
            {
              *separator = t->token_value->s[0];
              *digit = u->token_value->s[0];
              num_to_consume = 2;
            }
        }
    }

  consume_tokens (getter, num_to_consume, error_message);
}

static void
scan_decimal_integer (void *state, buffered_token_getter_t getter,
                      pratt_tables_t tables, token_t tok, token_t *lhs,
                      const char **error_message)
{
  *lhs = NULL;

  voidp_vector_t digits = NULL;
  uint32_t *d = XMALLOC (uint32_t);
  *d = tok->token_value->s[0];
  digits = voidp_vector_push (digits, d);

  uint32_t separator;
  uint32_t next_digit;
  get_next_digit (getter, &separator, &next_digit, error_message);
  while (*error_message == NULL && is_ascii_digit (next_digit))
    {
      if (uc_is_general_category (separator, UC_CATEGORY_Pc))
        {
          d = XMALLOC (uint32_t);
          *d = separator;
          digits = voidp_vector_push (digits, d);
        }

      d = XMALLOC (uint32_t);
      *d = next_digit;
      digits = voidp_vector_push (digits, d);

      if (*error_message == NULL)
        get_next_digit (getter, &separator, &next_digit, error_message);
    }

  if (*error_message == NULL)
    {
      struct string *str = XMALLOC (struct string);
      str->n = voidp_vector_length (digits);
      str->s = XNMALLOC (str->n, uint32_t);
      for (size_t i = 0; i != str->n; i += 1)
        str->s[i] = *((const uint32_t *) voidp_vector_ref (digits, i));

      *lhs = make_token_t (make_string_t ("I10"), str, tok->loc);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
nud_handler_t next_code_point_handler;

static void
code_point_handler (void *state, buffered_token_getter_t getter,
                    pratt_tables_t tables, token_t tok, token_t *lhs,
                    const char **error_message)
{
  if (*error_message == NULL)
    {
      if (token_is_ascii_digit (tok))
        scan_decimal_integer (state, getter, tables, tok, lhs,
                              error_message);
      else
        next_code_point_handler (state, getter, tables, tok, lhs,
                                 error_message);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////

nud_handler_t next_cp_handler;

static void
cp_handler (void *state, buffered_token_getter_t getter,
            pratt_tables_t tables, token_t tok, token_t *lhs,
            const char **error_message)
{
  if (*error_message == NULL)
    {
      if (token_is_ascii_digit (tok))
        scan_decimal_integer (state, getter, tables, tok, lhs,
                              error_message);
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

  //////////////////////////////////////////////////////////////////////////////////////////
  tables = lexical_pratt_tables ();
  next_code_point_handler = pratt_nud_get (tables, string_t_CP ());
  pratt_nud_put (tables, string_t_CP (), &code_point_handler);
  //////////////////////////////////////////////////////////////////////////////////////////

  tables =
    get_pratt_tables_for_pass ("100-scan-decimal-integer-without-sign");
  next_cp_handler = pratt_nud_get (tables, string_t_CP ());
  pratt_nud_put (tables, string_t_CP (), &cp_handler);
  set_pratt_tables_for_pass ("100-scan-decimal-integer-without-sign",
                             tables);

  release_pratt_tables_lock ();
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
