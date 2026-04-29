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

  Pratt parsing
  -------------

  We use Pratt ‘parsing’ not only for parsing but also for lexical
  analysis. For parsing, we run the Pratt parser in the usual way:
  tokens in, parse tree out. For lexical analysis, we feed tokens in
  and get tokens out, and do this repeatedly until the token stream
  reaches a fixed point. The initial token stream consists of Unicode
  code points (in NFC canonical form), and end-of-file markers.

*/

#include <config.h>
#include <math.h>
#include <inttypes.h>
#include <gl_avltree_omap.h>
#include <gl_xomap.h>
#include <libhiha/string_t.h>
#include <libhiha/load_plugin.h>
#include <libhiha/pratt.h>

// Change this if using gettext.
#define _(msgid) msgid

#define UNEXPECTED_TEXT _("unexpected text at %s: “%s”")

struct pratt_tables
{
  gl_omap_t nud;
  gl_omap_t led;
  gl_omap_t lbp;
};
typedef struct pratt_tables *pratt_tables_t;

typedef long int pratt_binding_power_t;
#define PRATT_BINDING_POWER_MIN LONG_MIN
#define PRATT_BINDING_POWER_MAX LONG_MAX

static pratt_binding_power_t
make_pratt_binding_power_t (double bp)
{
  /* There are six decimal digits available for binding powers. */

  pratt_binding_power_t result;

  bp = rint (bp * 1.0e6);
  if (bp <= PRATT_BINDING_POWER_MIN)
    result = PRATT_BINDING_POWER_MIN;
  else if (PRATT_BINDING_POWER_MAX <= bp)
    result = PRATT_BINDING_POWER_MAX;
  else
    result = lrint (bp);
  return result;
}

static int
binding_powers_lt (double a, double b)
{
  pratt_binding_power_t _a = make_pratt_binding_power_t (a);
  pratt_binding_power_t _b = make_pratt_binding_power_t (b);
  return (a < b);
}

static int
compare_strings (const void *s1, const void *s2)
{
  const string_t str1 = (const string_t) s1;
  const string_t str2 = (const string_t) s2;
  return string_t_cmp (str1, str2);
}

HIHA_VISIBLE pratt_tables_t
make_pratt_tables_t (void)
{
  pratt_tables_t data = XMALLOC (struct pratt_tables);
  data->nud =
    gl_omap_create_empty (GL_AVLTREE_OMAP, compare_strings, NULL, NULL);
  data->led =
    gl_omap_create_empty (GL_AVLTREE_OMAP, compare_strings, NULL, NULL);
  data->lbp =
    gl_omap_create_empty (GL_AVLTREE_OMAP, compare_strings, NULL, NULL);
  return data;
}

HIHA_VISIBLE void
pratt_nud_put (pratt_tables_t data, string_t token_kind,
               nud_handler_t handler)
{
  gl_omap_put (data->nud, token_kind, handler);
}

HIHA_VISIBLE void
pratt_led_put (pratt_tables_t data, string_t token_kind,
               led_handler_t handler)
{
  gl_omap_put (data->led, token_kind, handler);
}

HIHA_VISIBLE void
pratt_lbp_put (pratt_tables_t data, string_t token_kind,
               double binding_power)
{
  double *bp = XMALLOC (double);
  *bp = binding_power;
  gl_omap_put (data->lbp, token_kind, bp);
}

HIHA_VISIBLE nud_handler_t
pratt_nud_get (pratt_tables_t data, string_t token_kind)
{
  return (nud_handler_t) gl_omap_get (data->nud, token_kind);
}

HIHA_VISIBLE led_handler_t
pratt_led_get (pratt_tables_t data, string_t token_kind)
{
  return (led_handler_t) gl_omap_get (data->nud, token_kind);
}

HIHA_VISIBLE double
pratt_lbp_get (pratt_tables_t data, string_t token_kind)
{
  const void *p = gl_omap_get (data->lbp, token_kind);
  return (p == NULL) ? -HUGE_VAL : *((const double *) p);
}

static void
execute_null_denotation (void *state, buffered_token_getter_t getter,
                         pratt_tables_t tables, void **lhs,
                         const char **error_message)
{
  token_t tok;

  *lhs = NULL;
  //
  // Consume the next token.
  //
  getter->get_token (getter, &tok, error_message);
  if (*error_message == NULL)
    {
      nud_handler_t handler = pratt_nud_get (tables, tok->token_kind);
      if (handler == NULL)
        {
          //
          // There is no null denotation. Treat this as a lexical or
          // syntax error.
          //
          char s[1000];
          snprintf (s, 1000, UNEXPECTED_TEXT,
                    text_location_string (tok->loc),
                    make_str_nul (tok->token_value));
          *error_message = xstrdup (s);
        }
      else
        //
        // Success.
        //
        handler (state, getter, tables, tok, lhs, error_message);
    }
}

static void
peek_at_next_token (void *state, buffered_token_getter_t getter,
                    pratt_tables_t tables, double *left_binding_power,
                    const char **error_message)
{
  if (*error_message == NULL)
    {
      //
      // Peek at the next token, without consuming it.
      //
      token_t tok;
      getter->look_at_token (getter, 0, &tok, error_message);

      if (*error_message == NULL)
        *left_binding_power = pratt_lbp_get (tables, tok->token_kind);
    }
}

static void
execute_left_denotation (void *state, buffered_token_getter_t getter,
                         pratt_tables_t tables, void **lhs,
                         const char **error_message)
{
  token_t tok;

  //
  // Consume the next token.
  //
  getter->get_token (getter, &tok, error_message);
  if (error_message == NULL)
    {
      led_handler_t handler = pratt_led_get (tables, tok->token_kind);
      if (handler == NULL)
        {
          //
          // Unexpected token. Treat this as a lexical or syntax
          // error.
          //
          char s[1000];
          snprintf (s, 1000, UNEXPECTED_TEXT,
                    text_location_string (tok->loc),
                    make_str_nul (tok->token_value));
          *error_message = xstrdup (s);
        }
      else
        //
        // Success.
        //
        handler (state, getter, tables, tok, lhs, error_message);
    }
}

HIHA_VISIBLE void
pratt_parse (void *state, buffered_token_getter_t getter,
             pratt_tables_t tables, double min_power,
             void **lhs, const char **error_message)
{
  double binding_power;

  execute_null_denotation (state, getter, tables, lhs, error_message);
  peek_at_next_token (state, getter, tables, &binding_power,
                      error_message);
  while (*error_message == NULL
         && binding_powers_lt (min_power, binding_power))
    execute_left_denotation (state, getter, tables, lhs, error_message);
}

HIHA_VISIBLE pratt_handler_reference_t
make_pratt_handler_reference_t (size_t register_no, size_t handler_no)
{
  pratt_handler_reference_t ref =
    XMALLOC (struct pratt_handler_reference);
  ref->register_no = register_no;
  ref->handler_no = handler_no;
  return ref;
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
