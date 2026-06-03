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

  -----------
  IDENTIFIERS
  -----------

  Identifiers are Unicode ID_START followed by Unicode ID_CONTINUE.
  However, as in Ada, connectors such as underscore _ are allowed to
  appear only between other characters, and cannot be strung together.

  (Underscores often are easily misread in hardcopy.)

  Note, though, that this compiler may be using NFC normalization, and
  not NFKC normalization. Things that look alike may have different
  meanings. We make this choice because we like superscripts,
  subscripts, etc., to be distinct in meaning. They often are,
  presumably because many compilers and interpreters do not bother to
  normalize the Unicode at all.


*/

#include <config.h>
#include <error.h>
#include <exitfail.h>
#include <unictype.h>
#include <xalloc.h>
#include <libhiha/libhiha.h>

#define _(msgid) HIHA_GETTEXT (msgid)

static bool
is_identifier_start (uint32_t cp)
{
  return uc_is_property (cp, UC_PROPERTY_ID_START);
}

static bool
token_is_identifier_start (token_t tok)
{
  return (tok->token_value->n == 1
          && is_identifier_start (tok->token_value->s[0]));
}

static bool
is_connector (uint32_t cp)
{
  return uc_is_general_category (cp, UC_CATEGORY_Pc);
}

static bool
token_is_connector (token_t tok)
{
  return (tok->token_value->n == 1
          && is_connector (tok->token_value->s[0]));
}

static bool
is_identifier_continue (uint32_t cp)
{
  return (uc_is_property (cp, UC_PROPERTY_ID_CONTINUE)
          && !is_connector (cp));
}

static bool
token_is_identifier_continue (token_t tok)
{
  return (tok->token_value->n == 1
          && is_identifier_continue (tok->token_value->s[0]));
}

static void
consume_tokens (buffered_token_getter_t getter, size_t num_to_consume,
                const char **error_message)
{
  token_t t;
  size_t i = 0;
  while (*error_message == NULL && i != num_to_consume)
    {
      (void) getter->get_token (getter, &t, error_message);
      i += 1;
    }
}

static void
get_next_character (buffered_token_getter_t getter,
                    uint32_t *separator, uint32_t *character,
                    const char **error_message)
{
  *separator = 0;
  *character = 0;
  size_t num_to_consume = 0;

  token_t t;
  getter->look_at_token (getter, 0, &t, error_message);
  if (*error_message == NULL)
    {
      if (token_is_identifier_continue (t))
        {
          *character = t->token_value->s[0];
          num_to_consume = 1;
        }
      else if (token_is_connector (t))
        {
          token_t u;
          getter->look_at_token (getter, 1, &u, error_message);
          if (*error_message == NULL
              && token_is_identifier_continue (u))
            {
              *separator = t->token_value->s[0];
              *character = u->token_value->s[0];
              num_to_consume = 2;
            }
        }
    }

  consume_tokens (getter, num_to_consume, error_message);
}

static void
scan_identifier (void *state, buffered_token_getter_t getter,
                 pratt_tables_t tables, token_t tok, token_t *lhs,
                 const char **error_message)
{
  *lhs = NULL;

  voidp_vector_t characters = NULL;
  uint32_t *c = XMALLOC (uint32_t);
  *c = tok->token_value->s[0];
  characters = voidp_vector_push (characters, c);

  uint32_t separator;
  uint32_t next_character;
  get_next_character (getter, &separator, &next_character,
                      error_message);
  while (*error_message == NULL
         && is_identifier_continue (next_character))
    {
      if (is_connector (separator))
        {
          c = XMALLOC (uint32_t);
          *c = separator;
          characters = voidp_vector_push (characters, c);
        }

      c = XMALLOC (uint32_t);
      *c = next_character;
      characters = voidp_vector_push (characters, c);

      if (*error_message == NULL)
        get_next_character (getter, &separator, &next_character,
                            error_message);
    }

  if (*error_message == NULL)
    {
      struct string *str = XMALLOC (struct string);
      str->n = voidp_vector_length (characters);
      str->s = XNMALLOC (str->n, uint32_t);
      for (size_t i = 0; i != str->n; i += 1)
        str->s[i] =
          *((const uint32_t *) voidp_vector_ref (characters, i));

      *lhs = make_token_t (make_string_t ("ID"), str, tok->loc);
    }
}

nud_handler_t next_cp_handler;

static void
cp_handler (void *state, buffered_token_getter_t getter,
            pratt_tables_t tables, token_t tok, token_t *lhs,
            const char **error_message)
{
  if (*error_message == NULL)
    {
      if (token_is_identifier_start (tok))
        scan_identifier (state, getter, tables, tok, lhs,
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

  tables = get_pratt_tables_for_pass ("100-scan-identifiers");
  next_cp_handler = pratt_nud_get (tables, string_t_CP (), NULL);
  pratt_nud_put (tables, string_t_CP (), NULL, &cp_handler);
  set_pratt_tables_for_pass ("100-scan-identifiers", tables);

  release_pratt_tables_lock ();
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
