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
#include <libhiha/pratt.h>

// Change this if using gettext.
#define _(msgid) msgid

#define VISIBLE [[gnu::visibility ("default")]]

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

VISIBLE pratt_tables_t
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

VISIBLE void
pratt_add_nud (pratt_tables_t data, string_t token_kind,
               nud_handler_t handler)
{
  gl_omap_put (data->nud, token_kind, handler);
}

VISIBLE void
pratt_add_led (pratt_tables_t data, string_t token_kind,
               led_handler_t handler)
{
  gl_omap_put (data->led, token_kind, handler);
}

VISIBLE void
pratt_add_lbp (pratt_tables_t data, string_t token_kind,
               double binding_power)
{
  double *bp = XMALLOC (double);
  *bp = binding_power;
  gl_omap_put (data->lbp, token_kind, bp);
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
  if (error_message == NULL)
    {
      const void *p = gl_omap_get (tables->nud, tok->token_kind);
      if (p == NULL)
        {
          //
          // Syntax error.
          //
          char s[1000];
          snprintf (s, 1000, _("syntax error%s: “%s”"),
                    text_location_string (tok->loc),
                    make_str_nul (tok->token_value));
          *error_message = xstrdup (s);
        }
      else
        {
          //
          // Success.
          //
          nud_handler_t handler = (nud_handler_t) p;
          handler (state, tables, tok, lhs, error_message);
        }
    }
}

static void
peek_at_next_token (void *state, buffered_token_getter_t getter,
                    pratt_tables_t tables, double *left_binding_power,
                    const char **error_message)
{
  if (*error_message == NULL)
    {
      token_t tok;
      //
      // Peek at the next token, without consuming it.
      //
      getter->look_at_token (getter, 0, &tok, error_message);
      if (*error_message == NULL)
        {
          const void *p = gl_omap_get (tables->lbp, tok->token_kind);
          if (p == NULL)
            //
            // There is no entry in the binding power table. Default
            // to the absolute minimum binding power.
            //
            // Keep in mind that we do not consider binding powers to
            // have actual floating point values. They have fixed
            // point values with a certain number of decimal places.
            // That conversion happens elsewhere, however. The use of
            // “double” at this point, with later conversion to
            // decimal, is a concession to C compilers that do not
            // support _Decimal32 or _Decimal64.
            //
            // This all may seem like too much of a big deal, but I
            // was determined that binding powers should be exact
            // decimal numbers with a few useful decimal places.
            //
            *left_binding_power = -HUGE_VAL;
          else
            //
            // There is an entry in the binding power table. Use it.
            //
            *left_binding_power = *((const double *) p);
        }
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
      const void *p = gl_omap_get (tables->led, tok->token_kind);
      if (p == NULL)
        {
          //
          // Syntax error.
          //
          char s[1000];
          snprintf (s, 1000, _("syntax error%s: “%s”"),
                    text_location_string (tok->loc),
                    make_str_nul (tok->token_value));
          *error_message = xstrdup (s);
        }
      else
        {
          //
          // Success.
          //
          led_handler_t handler = (led_handler_t) p;
          handler (state, tables, tok, lhs, error_message);
        }
    }
}

VISIBLE void
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

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
