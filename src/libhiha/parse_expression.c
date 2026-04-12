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
#include <inttypes.h>
#include <math.h>
#include <gl_avltree_omap.h>
#include <gl_xomap.h>
#include <libhiha/string_t.h>
#include <libhiha/parse_expression.h>

// Change this if using gettext.
#define _(msgid) msgid

#define VISIBLE [[gnu::visibility ("default")]]

struct parser_data
{
  gl_omap_t nud;
  gl_omap_t led;
  gl_omap_t lbp;
};
typedef struct parser_data *parser_data_t;

typedef long int binding_power_t;

static binding_power_t
make_binding_power_t (double bp)
{
  /* There are six decimal digits available for binding powers. */

  binding_power_t result;

  bp = rint (bp * 1.0e6);
  if (bp <= LONG_MIN)
    result = LONG_MIN;
  else if (LONG_MAX <= bp)
    result = LONG_MAX;
  else
    result = lrint (bp);
  return result;
}

static int
compare_strings (const void *s1, const void *s2)
{
  const string_t str1 = (const string_t) s1;
  const string_t str2 = (const string_t) s2;
  return string_t_cmp (str1, str2);
}

VISIBLE parser_data_t
initialize_parser_data (void)
{
  parser_data_t data = XMALLOC (struct parser_data);
  data->nud =
    gl_omap_create_empty (GL_AVLTREE_OMAP, compare_strings, NULL, NULL);
  data->led =
    gl_omap_create_empty (GL_AVLTREE_OMAP, compare_strings, NULL, NULL);
  data->lbp =
    gl_omap_create_empty (GL_AVLTREE_OMAP, compare_strings, NULL, NULL);
  return data;
}

VISIBLE void
add_nud_entry (parser_data_t data, string_t token_kind,
	       nud_entry_handler_t handler)
{
  gl_omap_put (data->nud, token_kind, handler);
}

VISIBLE void
add_led_entry (parser_data_t data, string_t token_kind,
	       led_entry_handler_t handler)
{
  gl_omap_put (data->led, token_kind, handler);
}

VISIBLE void
add_lbp_entry (parser_data_t data, string_t token_kind,
	       double binding_power)
{
  binding_power_t *bp = XMALLOC (binding_power_t);
  *bp = make_binding_power_t (binding_power);
  gl_omap_put (data->lbp, token_kind, bp);
}

/*
parse_tree_t
parse_expression (xxxxxxx get_token,
                  parser_data_t data,
                  double min_power)
{
}
*/

/*
procedure parse_expression(get_token, tables, min_power)
   local tok, lhs

   /min_power := 0

   tok := get_token(1)
   lhs := tables.nud[tok.token_kind](tok)
   while min_power <
      tables.lbp[get_token(0).token_kind] do {
         tok := get_token(1)
         lhs := tables.led[tok.token_kind](lhs, tok)
      }
   return lhs
end

*/
/*//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  TOK_INTEGER,
  TOK_IDENTIFIER,
  TOK_PLUS,
  TOK_MINUS,
  TOK_TIMES,
  TOK_DIVIDE
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
