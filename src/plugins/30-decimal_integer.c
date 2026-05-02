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
#include <gl_xlist.h>
#include <gl_avltree_list.h>
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

static bool
is_ascii_digit (uint32_t cp)
{
  return ('0' <= cp && cp <= '9');
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
  if (*error_message == NULL
      && string_t_cmp (t->token_kind, string_t_CP ()) == 0)
    {
      check_code_point_token (t);
      if (is_ascii_digit (t->token_value->s[0]))
        {
          *digit = t->token_value->s[0];
          num_to_consume = 1;
        }
      else
        if (uc_is_general_category
            (t->token_value->s[0], UC_CATEGORY_Pc))
        {
          token_t u;
          getter->look_at_token (getter, 1, &u, error_message);
          if (*error_message == NULL
              && string_t_cmp (u->token_kind, string_t_CP ()) == 0)
            {
              check_code_point_token (u);
              if (is_ascii_digit (u->token_value->s[0]))
                {
                  *separator = t->token_value->s[0];
                  *digit = u->token_value->s[0];
                  num_to_consume = 2;
                }
            }
        }
    }

  consume_tokens (getter, num_to_consume, error_message);
}

static void
scan_decimal_integer (void *state, buffered_token_getter_t getter,
                      pratt_tables_t tables, token_t tok, void **lhs,
                      const char **error_message)
{
  *lhs = NULL;

  gl_list_t digits =
    gl_list_create_empty (GL_AVLTREE_LIST, NULL, NULL, NULL, true);
  uint32_t *d = XMALLOC (uint32_t);
  *d = tok->token_value->s[0];
  gl_list_add_last (digits, d);

  uint32_t separator;
  uint32_t next_digit;
  get_next_digit (getter, &separator, &next_digit, error_message);
  while (*error_message == NULL && is_ascii_digit (next_digit))
    {
      if (uc_is_general_category (separator, UC_CATEGORY_Pc))
        {
          d = XMALLOC (uint32_t);
          *d = separator;
          gl_list_add_last (digits, d);
        }

      d = XMALLOC (uint32_t);
      *d = next_digit;
      gl_list_add_last (digits, d);

      if (*error_message == NULL)
        get_next_digit (getter, &separator, &next_digit, error_message);
    }

  if (*error_message == NULL)
    {
      string_t str = XMALLOC (struct string);
      str->n = gl_list_size (digits);
      str->s = XNMALLOC (str->n, uint32_t);
      for (size_t i = 0; i != str->n; i += 1)
        str->s[i] = *((const uint32_t *) gl_list_get_at (digits, i));

      *lhs =
        (void *) make_token_t (make_string_t ("I10"), str, tok->loc);
    }
}

nud_handler_t next_cp_handler;

static void
code_point_handler (void *state, buffered_token_getter_t getter,
                    pratt_tables_t tables, token_t tok, void **lhs,
                    const char **error_message)
{
  bool done = (*error_message != NULL);
  if (!done)
    {
      check_code_point_token (tok);
      if (is_ascii_digit (tok->token_value->s[0]))
        {
          scan_decimal_integer (state, getter, tables, tok, lhs,
                                error_message);
          done = true;
        }
    }
  if (!done)
    next_cp_handler (state, getter, tables, tok, lhs, error_message);
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
