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
#include <libhiha/parse_expression.h>


/*
  # Pratt parsing for Icon.
#
# FIXME: Document this and make it available to the Icon and Unicon
# communities. Maybe make a class version.
#
# FIXME: Make an Object Icon version and add it to the distribution. Maybe
# make a class version.
#

record token_record(token_kind, token_value)
record pratt_parser_tables(nud, led, lbp)

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

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
