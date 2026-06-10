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
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <error.h>
#include <exitfail.h>
#include <quotearg.h>
#include <libhiha/libhiha.h>

#define _(msgid) HIHA_GETTEXT (msgid)

int64_t node_counter = 0;

static string_t str_KW;
static string_t str_I10;
static string_t str_SEQ;

static string_t str_open_paren;
static string_t str_close_paren;
static string_t str_begin;
static string_t str_if;
static string_t str_while;
static string_t str_end;

static token_t tok_SEQ;
static token_t tok_close_paren;
static token_t tok_end;

bool use_getentropy;

static bool
getentropy_works (void)
{
  char buf[256];
  int retval = getentropy (buf, 256);
  return (retval == 0);
}

static size_t
random_by_getentropy (void)
{
  unsigned long n;
  uint8_t buf[sizeof (unsigned long)];

  getentropy (buf, sizeof (unsigned long));
  memcpy (&n, buf, sizeof (unsigned long));
  double x = ((double) n) / ((double) ULONG_MAX);
  return x;
}

static size_t
random_by_portable_method (void)
{
  long n = random ();
  double x = ((double) n) / ((double) LONG_MAX);
  return x;
}

static size_t
random_size_t (size_t modulus)
{
  double x;
  if (use_getentropy)
    x = random_by_getentropy ();
  else
    x = random_by_portable_method ();
  size_t n = (size_t) (x * modulus);

  /* Let the random number generator be one that might return a random
     floating point number equal to 1. */
  if (n == modulus)
    n -= 1;

  return n;
}

static int64_t
next_node_number (void)
{
  node_counter += 1;
  return node_counter;
}

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

static bool
kw_token_is_nondeterministic (token_t tok)
{
  return (!deterministic
          && (string_t_cmp (tok->token_value, str_if) == 0
              || string_t_cmp (tok->token_value, str_while) == 0));
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
shuffle_SEQ (struct token_extension_for_parse_tree *extension)
{
  /* Shuffle the order of execution, to enforce nondeterminism.
     Otherwise programmers might start to rely on fortuitous
     deterministic behavior and thereby introduce bugs. */

  /* A Fisher-Yates shuffle. */
  for (size_t i = extension->num_children - 1; i != 0; i -= 1)
    {
      size_t j = random_size_t (i + 1);
      int64_t tmp = extension->children[i];
      extension->children[i] = extension->children[j];
      extension->children[j] = tmp;
    }
}

static token_t
closing_paren (string_t tokval)
{
  return ((string_t_cmp (tokval, str_open_paren) == 0)
          ? tok_close_paren : tok_end);
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
      token_t closing = closing_paren (tok->token_value);
      token_t t;
      getter->get_token (getter, &t, error_message);
      if (*error_message == NULL)
        if (token_t_cmp (t, closing) == 0)
          {
            getter->push_back_token (getter, parenthetic_lhs,
                                     error_message);
            int64_t this_node = next_node_number ();
            struct token_extension_for_parse_tree *p =
              (struct token_extension_for_parse_tree *)
              parenthetic_lhs->extension;
            p->parent = this_node;
            if (*error_message == NULL)
              {
                int64_t children[1] = {
                  [0] = p->this
                };
                ((struct token *) lhs)->extension =
                  make_token_extension_for_parse_tree
                  (1, children, this_node);
                if (token_t_cmp (*lhs, tok_SEQ) == 0
                    && kw_token_is_nondeterministic (tok))
                  shuffle_SEQ (p);
              }
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
    parenthetic_handler (state, getter, tables, tok, lhs,
                         error_message);
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
  str_SEQ = make_string_t ("seq");

  str_open_paren = make_string_t ("(");
  str_close_paren = make_string_t (")");
  str_begin = make_string_t ("begin");
  str_if = make_string_t ("if");
  str_while = make_string_t ("while");
  str_end = make_string_t ("end");

  tok_close_paren = make_token_t (str_KW, str_close_paren, NULL);
  tok_end = make_token_t (str_KW, str_end, NULL);
  tok_SEQ = make_token_t (str_SEQ, str_SEQ, NULL);
}

HIHA_VISIBLE void
plugin_init (void)
{
  initialize_strings_and_tokens ();
  use_getentropy = getentropy_works ();

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
