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
