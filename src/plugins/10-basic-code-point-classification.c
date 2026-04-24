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

VISIBLE volatile void
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

VISIBLE volatile void
code_point_handler (void *state, pratt_tables_t tables,
                    token_t tok, void **lhs, const char **error_message)
{
  check_code_point_token (tok);
  *error_message = NULL;
  uint32_t cp = tok->token_value->s[0];
  if (uc_is_property_white_space (cp))
    *lhs =
      (void *) make_token_t (make_string_t ("SP"), tok->token_value,
                             tok->loc);
  else if (is_ascii_digit (cp))
    *lhs =
      (void *) make_token_t (make_string_t ("0-9"), tok->token_value,
                             tok->loc);
  else if (uc_is_property_alphabetic (cp))
    *lhs =
      (void *) make_token_t (make_string_t ("AL"), tok->token_value,
                             tok->loc);
  else if (uc_is_property_quotation_mark (cp))
    *lhs =
      (void *) make_token_t (make_string_t ("QU"), tok->token_value,
                             tok->loc);
  else
    *lhs = (void *) tok;
}

VISIBLE volatile plugin_interface_t
plugin_init (const char *filename, size_t register_no)
{
  enum
  {
    i_EOF_HANDLER = 0,
    i_CODE_POINT_HANDLER = 1,
    i_END = 2
  };

  void **nud_handlers = XCALLOC (i_END + 1, void *);
  nud_handlers[i_EOF_HANDLER] = xstrdup ("eof_handler");
  nud_handlers[i_CODE_POINT_HANDLER] = xstrdup ("code_point_handler");
  nud_handlers[i_END] = NULL;

  plugin_pratt_interface_t pratt_interface =
    XMALLOC (struct plugin_pratt_interface);
  pratt_interface->nud_handlers = nud_handlers;
  pratt_interface->led_handlers = NULL;

  plugin_interface_t interface = XMALLOC (struct plugin_interface);
  interface->filename = filename;
  interface->register_no = register_no;
  interface->tag = tag_PLUGIN_PRATT_INTERFACE;
  interface->interface = pratt_interface;

  pratt_tables_t tables = lexical_pratt_tables ();
  pratt_nud_put (tables, string_t_EOF (),
                 make_pratt_handler_reference_t (register_no,
                                                 i_EOF_HANDLER));
  pratt_nud_put (tables, string_t_CP (),
                 make_pratt_handler_reference_t (register_no,
                                                 i_CODE_POINT_HANDLER));
  return interface;
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
