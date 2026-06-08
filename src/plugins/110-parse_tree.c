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
#include <errno.h>
#include <error.h>
#include <exitfail.h>
#include <quotearg.h>
#include <libhiha/libhiha.h>

#define _(msgid) HIHA_GETTEXT (msgid)

static string_t str_KW;
static string_t str_I10;

static string_t str_open_paren;
static string_t str_close_paren;
static string_t str_begin;
static string_t str_if;
static string_t str_while;
static string_t str_end;

static token_t tok_close_paren;
static token_t tok_end;

static bool
token_breaks_up_juxtaposition (token_t tok)
{
  bool b = (tok->token_value->n == 1);
  if (b)
    {
      uint32_t u = tok->token_value->s[0];
      b = (u == '.' || u == ';');
    }
  return b;
}

static bool
kw_token_is_parenthetic (token_t tok)
{
  return (string_t_cmp (tok->token_value, str_open_paren) == 0
          || string_t_cmp (tok->token_value, str_begin) == 0
          || string_t_cmp (tok->token_value, str_if) == 0
          || string_t_cmp (tok->token_value, str_while) == 0);
};

static void
default_handler (void *state, buffered_token_getter_t getter,
                 pratt_tables_t tables, token_t tok,
                 token_t *lhs, const char **error_message)
{
  if (*error_message == NULL)
    {
      error (0, 0, "syntax error: %s", text_location_string (tok->loc));

      size_t n_u8;
      uint8_t *u8 =
        u32_to_u8 (tok->token_value->s, tok->token_value->n, NULL,
                   &n_u8);
      int err_number = errno;
      if (u8 == NULL)
        {
          error (exit_failure, err_number, "");
          abort ();
        }
      char *s = (char *) u8;
      struct quoting_options *qu_opts = clone_quoting_options (NULL);
      set_quoting_style (qu_opts, locale_quoting_style);
      error (0, 0, "syntax error: %s",
             quotearg_alloc (s, strlen (s), qu_opts));

      exit (exit_failure);
    }
}

static void
parenthetic_handler (void *state, buffered_token_getter_t getter,
                     pratt_tables_t tables, token_t tok,
                     token_t *lhs, const char **error_message)
{
  token_t parenthetic_lhs;
  pratt_parse (state, getter, tables, minimum_binding_power (),
               &parenthetic_lhs, error_message);
  if (*error_message == NULL)
    {
      token_t closing =
        (string_t_cmp (tok->token_value, str_open_paren) == 0)
        ? tok_close_paren : tok_end;
      token_t t;
      getter->get_token (getter, &t, error_message);
      if (*error_message == NULL)
        if (token_t_cmp (t, closing) == 0)
          {
            // FIXME: BUILD OUR OWN TOKEN BASED ON parenthetic_lhs.
            *lhs = parenthetic_lhs;
          }
        else
          {
            char buf[1000];
            snprintf (buf, 1000, _("%s: expected “%s”"),
                      text_location_string (t->loc),
                      make_str_nul (closing->token_value));
            *error_message = xstrdup (buf);
          }
    }
}

static void
kw_handler (void *state, buffered_token_getter_t getter,
            pratt_tables_t tables, token_t tok,
            token_t *lhs, const char **error_message)
{
  if (kw_token_is_parenthetic (tok))
    parenthetic_handler (state, getter, tables, tok, lhs, error_message);
  else
    default_handler (state, getter, tables, tok, lhs, error_message);
}

//// static void
//// reference_handler (void *state, buffered_token_getter_t getter,
////                    pratt_tables_t tables, token_t tok,
////                    token_t *lhs, const char **error_message)
//// {
////   if (*error_message == NULL)
////     {
////       token_t t;
////       getter->look_at_token (getter, 0, &t, error_message);
////       if (*error_message == NULL)
////         {
////           
////         }
////     }
//// }

static void
initialize_strings_and_tokens (void)
{
  str_KW = string_t_KW ();
  str_I10 = make_string_t ("I10");

  str_open_paren = make_string_t ("(");
  str_close_paren = make_string_t (")");
  str_begin = make_string_t ("begin");
  str_if = make_string_t ("if");
  str_while = make_string_t ("while");
  str_end = make_string_t ("end");

  tok_close_paren = make_token_t (str_KW, str_close_paren, NULL);
  tok_end = make_token_t (str_KW, str_end, NULL);
}

HIHA_VISIBLE void
plugin_init (void)
{
  initialize_strings_and_tokens ();

  pratt_tables_t tables;

  acquire_pratt_tables_lock ();

  tables = get_pratt_tables_for_pass ("2000-parse-tree");
  pratt_nud_put_default (tables, &default_handler);
  pratt_nud_put (tables, str_KW, NULL, &kw_handler);
  ////////pratt_nud_put (tables, str_I10, NULL, &reference_handler);
  set_pratt_tables_for_pass ("2000-parse-tree", tables);

  release_pratt_tables_lock ();
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
