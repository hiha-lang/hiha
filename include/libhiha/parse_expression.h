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

#include <libhiha/string_t.h>
#include <libhiha/token_t.h>

struct parse_tree
{
  token_t token;
  size_t nchildren;
  struct parse_tree **children;
};
typedef struct parse_tree *parse_tree_t;

struct parser_data;
typedef struct parser_data *parser_data_t;

void parse_tree_t_free (parse_tree_t);

parser_data_t initialize_parser_data (void);
void parser_data_t_free (parser_data_t);

typedef parse_tree_t nud_entry_handler (parser_data_t, token_t);
typedef nud_entry_handler *nud_entry_handler_t;

typedef parse_tree_t led_entry_handler (parser_data_t, parse_tree_t,
					token_t);
typedef led_entry_handler *led_entry_handler_t;

void add_nud_entry (parser_data_t data, string_t token_kind,
		    nud_entry_handler_t);
void add_led_entry (parser_data_t data, string_t token_kind,
		    led_entry_handler_t);
void add_lbp_entry (parser_data_t data, string_t token_kind,
		    _Decimal32 binding_power);

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
