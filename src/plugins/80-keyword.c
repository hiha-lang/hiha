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
  "end",
  "begin",
  "do",
  "while",
  ".",
  ";"
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

nud_handler_t next_cp_handler;
nud_handler_t next_id_handler;
nud_handler_t next_sy_handler;

#define DEFINE_HANDLER(XX)                                      \
  static void                                                   \
  XX##_handler (void *state, buffered_token_getter_t getter,    \
                pratt_tables_t tables, token_t tok,             \
                token_t *lhs, const char **error_message)       \
  {                                                             \
    if (*error_message == NULL)                                 \
      {                                                         \
        if (is_keyword (tok))                                   \
          *lhs =                                                \
            make_token_t (str_KW, tok->token_value, tok->loc);  \
        else                                                    \
          next_##XX##_handler (state, getter, tables, tok, lhs, \
                               error_message);                  \
      }                                                         \
  }

DEFINE_HANDLER (cp);
DEFINE_HANDLER (sy);
DEFINE_HANDLER (id);

HIHA_VISIBLE void
plugin_init (void)
{
  initialize_keywords ();
  str_KW = make_string_t ("KW");
  str_CP = string_t_CP ();
  str_SY = make_string_t ("SY");
  str_ID = make_string_t ("ID");

  pratt_tables_t tables;

  acquire_pratt_tables_lock ();

  tables = get_pratt_tables_for_pass ("500-mark-keywords");
  next_cp_handler = pratt_nud_get (tables, str_CP, NULL);
  next_sy_handler = pratt_nud_get (tables, str_SY, NULL);
  next_id_handler = pratt_nud_get (tables, str_ID, NULL);
  pratt_nud_put (tables, str_CP, NULL, &cp_handler);
  pratt_nud_put (tables, str_SY, NULL, &sy_handler);
  pratt_nud_put (tables, str_ID, NULL, &id_handler);
  set_pratt_tables_for_pass ("500-mark-keywords", tables);

  release_pratt_tables_lock ();
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
