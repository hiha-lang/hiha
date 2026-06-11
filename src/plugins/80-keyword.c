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
#include <xalloc.h>
#include <libhiha/libhiha.h>

#define _(msgid) HIHA_GETTEXT (msgid)

static const char *kw[] = {
  ";",
  "(",
  ")",
  "end",
  "begin",
  "if",
  "infix_left",
  "infix_right",
  "postfix",
  "prefix",
  "procedure",
  "then",
  "while"
};

static string_t str_KW;
static string_t str_CP;
static string_t str_SY;
static string_t str_ID;

static size_t num_keywords;
static string_t *keywords;

static void
initialize_keywords (void)
{
  num_keywords = sizeof kw / sizeof (const char *);
  keywords = XNMALLOC (num_keywords, string_t);
  size_t i = 0;
  while (i != num_keywords)
    {
      keywords[i] = (kw[i] == NULL) ? NULL : (make_string_t (kw[i]));
      i += 1;
    }
}

static bool
is_keyword (token_t tok)
{
  size_t i = 0;
  while (i != num_keywords
         && (keywords[i] == NULL
             || string_t_cmp (tok->token_value, keywords[i]) != 0))
    i += 1;
  return (i != num_keywords);
}

nud_handler_t next_handler;

static void
handler (void *state, buffered_token_getter_t getter,
         pratt_tables_t tables, token_t tok,
         token_t *lhs, const char **error_message)
{
  *error_message = NULL;
  if (is_keyword (tok))
    *lhs = make_token_t (str_KW, tok->token_value, tok->loc);
  else
    next_handler (state, getter, tables, tok, lhs, error_message);
}

HIHA_VISIBLE void
plugin_init (void)
{
  initialize_keywords ();
  str_KW = string_t_KW ();
  str_CP = string_t_CP ();
  str_SY = make_string_t ("SY");
  str_ID = make_string_t ("ID");

  pratt_tables_t tables;

  acquire_pratt_tables_lock ();

  tables = get_pratt_tables_for_pass ("500.100-mark-keywords");
  next_handler = pratt_nud_get (tables, str_CP, NULL);
  pratt_nud_put (tables, str_CP, NULL, &handler);
  set_pratt_tables_for_pass ("500.100-mark-keywords", tables);

  tables = get_pratt_tables_for_pass ("500.200-mark-keywords");
  next_handler = pratt_nud_get (tables, str_SY, NULL);
  pratt_nud_put (tables, str_SY, NULL, &handler);
  set_pratt_tables_for_pass ("500.200-mark-keywords", tables);

  tables = get_pratt_tables_for_pass ("500.300-mark-keywords");
  next_handler = pratt_nud_get (tables, str_ID, NULL);
  pratt_nud_put (tables, str_ID, NULL, &handler);
  set_pratt_tables_for_pass ("500.300-mark-keywords", tables);

  release_pratt_tables_lock ();
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
