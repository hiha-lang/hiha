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
is_identifier_start (uint32_t cp)
{
  return uc_is_property (cp, UC_PROPERTY_ID_START);
}

static bool
is_connector (uint32_t cp)
{
  return uc_is_general_category (cp, UC_CATEGORY_Pc);
}

static bool
is_identifier_continue (uint32_t cp)
{
  return (uc_is_property (cp, UC_PROPERTY_ID_CONTINUE)
          && !is_connector (cp));
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
  if (*error_message == NULL
      && string_t_cmp (t->token_kind, string_t_CP ()) == 0)
    {
      check_code_point_token (t);
      if (is_identifier_continue (t->token_value->s[0]))
        {
          *character = t->token_value->s[0];
          num_to_consume = 1;
        }
      else if (is_connector (t->token_value->s[0]))
        {
          token_t u;
          getter->look_at_token (getter, 1, &u, error_message);
          if (*error_message == NULL
              && string_t_cmp (u->token_kind, string_t_CP ()) == 0)
            {
              check_code_point_token (u);
              if (is_identifier_continue (u->token_value->s[0]))
                {
                  *separator = t->token_value->s[0];
                  *character = u->token_value->s[0];
                  num_to_consume = 2;
                }
            }
        }
    }

  consume_tokens (getter, num_to_consume, error_message);
}

static void
scan_identifier (void *state, buffered_token_getter_t getter,
                 pratt_tables_t tables, token_t tok, void **lhs,
                 const char **error_message)
{
  *lhs = NULL;

  gl_list_t characters =
    gl_list_create_empty (GL_AVLTREE_LIST, NULL, NULL, NULL, true);
  uint32_t *c = XMALLOC (uint32_t);
  *c = tok->token_value->s[0];
  gl_list_add_last (characters, c);

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
          gl_list_add_last (characters, c);
        }

      c = XMALLOC (uint32_t);
      *c = next_character;
      gl_list_add_last (characters, c);

      if (*error_message == NULL)
        get_next_character (getter, &separator, &next_character,
                            error_message);
    }

  if (*error_message == NULL)
    {
      string_t str = XMALLOC (struct string);
      str->n = gl_list_size (characters);
      str->s = XNMALLOC (str->n, uint32_t);
      for (size_t i = 0; i != str->n; i += 1)
        str->s[i] =
          *((const uint32_t *) gl_list_get_at (characters, i));

      *lhs =
        (void *) make_token_t (make_string_t ("ID"), str, tok->loc);
    }
}

nud_handler_t next_handler;

static void
code_point_handler (void *state, buffered_token_getter_t getter,
                    pratt_tables_t tables, token_t tok, void **lhs,
                    const char **error_message)
{
  if (*error_message == NULL)
    {
      check_code_point_token (tok);
      if (is_identifier_start (tok->token_value->s[0]))
        scan_identifier (state, getter, tables, tok, lhs,
                         error_message);
      else
        next_handler (state, getter, tables, tok, lhs, error_message);
    }
}

HIHA_VISIBLE void
plugin_init (void)
{
  pratt_tables_t tables = lexical_pratt_tables ();
  next_handler = pratt_nud_get (tables, string_t_CP ());
  pratt_nud_put (tables, make_string_t ("CP"), &code_point_handler);
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
