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
static string_t str_NJUX;
static string_t str_SEQ;
static string_t str_BEGIN;
static string_t str_WHILE;
static string_t str_IF;
static string_t str_EOF;

static string_t str_open_paren;
static string_t str_close_paren;
static string_t str_begin;
static string_t str_if;
static string_t str_while;
static string_t str_end;

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
  return (tok->token_value->n == 1
          && tok->token_value->s[0] == ';'
          && string_t_cmp (tok->token_kind, str_KW) == 0);
}

static bool
token_ends_sequence (token_t tok)
{
  return (string_t_cmp (tok->token_kind, string_t_EOF ()) == 0
          || token_t_cmp (tok, tok_close_paren) == 0
          || token_t_cmp (tok, tok_end) == 0);
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
passthrough_handler (void *state, buffered_token_getter_t getter,
                     token_putter_t putter, pratt_tables_t tables,
                     token_t tok, token_t *lhs,
                     const char **error_message)
{
  *error_message = NULL;
  *lhs = tok;
}

static void
default_handler (void *state, buffered_token_getter_t getter,
                 token_putter_t putter, pratt_tables_t tables,
                 token_t tok, token_t *lhs, const char **error_message)
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

static string_t
sequence_kind (string_t str)
{
  string_t s = NULL;
  if (string_t_cmp (str, str_begin) == 0
      || string_t_cmp (str, str_open_paren) == 0)
    s = str_BEGIN;
  else if (string_t_cmp (str, str_while) == 0)
    s = str_WHILE;
  else if (string_t_cmp (str, str_if) == 0)
    s = str_IF;
  assert (s != NULL);
  return s;
}

static void
parenthetic_handler (void *state, buffered_token_getter_t getter,
                     token_putter_t putter, pratt_tables_t tables,
                     token_t tok, token_t *lhs,
                     const char **error_message)
{
  token_t parenthetic_lhs;
  pratt_parse (state, getter, putter, tables, minimum_binding_power (),
               &parenthetic_lhs, error_message);
  if (*error_message == NULL)
    {
      token_t closing = closing_paren (tok->token_value);
      token_t t;
      getter->get_token (getter, &t, error_message);
      if (*error_message == NULL)
        if (token_t_cmp (t, closing) == 0)
          {
            struct token_extension_for_parse_tree *p =
              (struct token_extension_for_parse_tree *)
              parenthetic_lhs->extension;
            string_t tokkind = sequence_kind (tok->token_value);
            if (0 == string_t_cmp (parenthetic_lhs->token_kind,
                                   str_SEQ))
              {
                if (kw_token_is_nondeterministic (tok))
                  shuffle_SEQ (p);
                *lhs =
                  make_extended_token_t (tokkind,
                                         parenthetic_lhs->token_value,
                                         parenthetic_lhs->loc,
                                         parenthetic_lhs->extension);
              }
            else
              {
                int64_t this_node = next_node_number ();
                p->parent = this_node;
                int64_t children[1] = {
                  [0] = p->this
                };
                putter->put_token (putter, parenthetic_lhs,
                                   error_message);
                if (*error_message == NULL)
                  *lhs =
                    make_extended_token_t (tokkind,
                                           parenthetic_lhs->token_value,
                                           parenthetic_lhs->loc,
                                           make_token_extension_for_parse_tree
                                           (1, children, this_node));
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
start_sequence (void *state, buffered_token_getter_t getter,
                token_putter_t putter, pratt_tables_t tables,
                token_t tok, token_t *lhs, const char **error_message)
{
  token_t rhs;
  pratt_parse (state, getter, putter, tables, njux_binding_power, &rhs,
               error_message);
  if (*error_message == NULL)
    {
      struct token_extension_for_parse_tree *p =
        (struct token_extension_for_parse_tree *) rhs->extension;
      struct token_extension_for_parse_tree *q =
        (struct token_extension_for_parse_tree *) (*lhs)->extension;
      int64_t children[2] = {
        [0] = q->this,
        [1] = p->this
      };
      int64_t this_node = next_node_number ();
      token_extension_t ext =
        make_token_extension_for_parse_tree (2, children, this_node);
      q->parent = this_node;
      p->parent = this_node;
      putter->put_token (putter, *lhs, error_message);
      if (*error_message == NULL)
        putter->put_token (putter, rhs, error_message);
      if (*error_message == NULL)
        {
          string_t tokval =
            concat_string_t ((*lhs)->token_value, tok->token_value,
                             rhs->token_value, NULL);
          *lhs =
            make_extended_token_t (str_SEQ, tokval, (*lhs)->loc, ext);
        }
    }
}

static void
extend_sequence (void *state, buffered_token_getter_t getter,
                 token_putter_t putter, pratt_tables_t tables,
                 token_t tok, token_t *lhs, const char **error_message)
{
  token_t rhs;
  pratt_parse (state, getter, putter, tables, njux_binding_power, &rhs,
               error_message);
  if (*error_message == NULL)
    {
      struct token_extension_for_parse_tree *p =
        (struct token_extension_for_parse_tree *) rhs->extension;
      struct token_extension_for_parse_tree *q =
        (struct token_extension_for_parse_tree *) (*lhs)->extension;
      p->parent = q->this;
      putter->put_token (putter, rhs, error_message);
      if (*error_message == NULL)
        {
          size_t num_children = q->num_children + 1;
          int64_t *children = XNMALLOC (num_children, int64_t);
          memcpy (children, q->children,
                  q->num_children * sizeof (int64_t));
          children[num_children - 1] = p->this;
          token_extension_t ext =
            make_token_extension_for_parse_tree (num_children, children,
                                                 q->this);
          string_t tokval =
            concat_string_t ((*lhs)->token_value, tok->token_value,
                             rhs->token_value, NULL);
          *lhs =
            make_extended_token_t ((*lhs)->token_kind, tokval,
                                   (*lhs)->loc, ext);
        }
    }
}

static void
end_sequence (void *state, buffered_token_getter_t getter,
              token_putter_t putter, pratt_tables_t tables, token_t tok,
              token_t *lhs, const char **error_message)
{
  /* Do nothing. */
}

static void
njux_handler (void *state, buffered_token_getter_t getter,
              token_putter_t putter, pratt_tables_t tables, token_t tok,
              token_t *lhs, const char **error_message)
{
  /* Consume extra non-juxtaposition tokens. */
  token_t t;
  getter->look_at_token (getter, 0, &t, error_message);
  while (string_t_cmp (t->token_kind, str_NJUX) == 0)
    {
      getter->get_token (getter, &t, error_message);
      if (*error_message == NULL)
        getter->look_at_token (getter, 0, &t, error_message);
    }

  if (*error_message == NULL)
    {
      if (token_ends_sequence (t))
        end_sequence (state, getter, putter, tables, tok, lhs,
                      error_message);
      else if (string_t_cmp ((*lhs)->token_kind, str_SEQ) == 0)
        extend_sequence (state, getter, putter, tables, tok, lhs,
                         error_message);
      else
        start_sequence (state, getter, putter, tables, tok, lhs,
                        error_message);
    }
}

static void
kw_handler_200 (void *state, buffered_token_getter_t getter,
                token_putter_t putter, pratt_tables_t tables,
                token_t tok, token_t *lhs, const char **error_message)
{
  if (kw_token_is_parenthetic (tok))
    parenthetic_handler (state, getter, putter, tables, tok, lhs,
                         error_message);
  else
    default_handler (state, getter, putter, tables, tok, lhs,
                     error_message);
}

static void
reference_handler (void *state, buffered_token_getter_t getter,
                   token_putter_t putter, pratt_tables_t tables,
                   token_t tok, token_t *lhs,
                   const char **error_message)
{
  *error_message = NULL;
  int64_t this_node = next_node_number ();
  int64_t children[1] = { };
  *lhs =
    make_extended_token_t
    (tok->token_kind, tok->token_value, tok->loc,
     make_token_extension_for_parse_tree (0, children, this_node));
}

nud_handler_t next_kw_handler_100;

static void
kw_handler_100 (void *state, buffered_token_getter_t getter,
                token_putter_t putter, pratt_tables_t tables,
                token_t tok, token_t *lhs, const char **error_message)
{
  if (token_breaks_up_juxtaposition (tok))
    *lhs = make_token_t (str_NJUX, tok->token_value, tok->loc);
  else
    next_kw_handler_100 (state, getter, putter, tables, tok, lhs,
                         error_message);
}

static void
initialize_strings_and_tokens (void)
{
  str_KW = string_t_KW ();
  str_I10 = make_string_t ("I10");
  str_NJUX = make_string_t ("NJUX");
  str_SEQ = make_string_t ("SEQ");
  str_BEGIN = make_string_t ("BEGIN");
  str_WHILE = make_string_t ("WHILE");
  str_IF = make_string_t ("IF");
  str_EOF = string_t_EOF ();

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
  use_getentropy = getentropy_works ();

  pratt_tables_t tables;

  acquire_pratt_tables_lock ();
  tables = get_pratt_tables_for_pass ("2000.100-non-juxtaposition");
  next_kw_handler_100 = pratt_nud_get (tables, str_KW, NULL);
  pratt_nud_put (tables, str_KW, NULL, &kw_handler_100);
  set_pratt_tables_for_pass ("2000.100-non-juxtaposition", tables);
  release_pratt_tables_lock ();

  acquire_pratt_tables_lock ();
  tables = get_pratt_tables_for_pass ("2000.200-parse-tree");
  pratt_nud_put_default (tables, &default_handler);
  pratt_nud_put (tables, str_EOF, NULL, &passthrough_handler);
  pratt_nud_put (tables, str_KW, NULL, &kw_handler_200);
  pratt_nud_put (tables, str_I10, NULL, &reference_handler);
  pratt_led_put (tables, str_NJUX, NULL, &njux_handler);
  pratt_lbp_put (tables, str_NJUX, NULL, njux_binding_power);
  set_pratt_tables_for_pass ("2000.200-parse-tree", tables);
  release_pratt_tables_lock ();
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
