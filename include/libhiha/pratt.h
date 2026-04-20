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

#ifndef __LIBHAHA__PRATT_H__INCLUDED__
#define __LIBHAHA__PRATT_H__INCLUDED__

#include <libhiha/string_t.h>
#include <libhiha/token_t.h>

struct pratt_tables;
typedef struct pratt_tables *pratt_tables_t;

pratt_tables_t make_pratt_tables_t (void);

/**INDENT-OFF**/
typedef void nud_handler (void *state, pratt_tables_t tables,
                          token_t tok, void **lhs,
                          const char **error_handler)
  [[gnu::access (write_only, 4)]];
/**INDENT-ON**/
typedef nud_handler *nud_handler_t;

typedef void led_handler (void *state, pratt_tables_t tables,
                          token_t tok, void **lhs,
                          const char **error_handler);
typedef led_handler *led_handler_t;

/* Add a null denotation handler to the Pratt parsing. */
void add_nud (pratt_tables_t data, string_t token_kind, nud_handler_t);

/* Add a left denotation handler to the Pratt parsing. */
void add_led (pratt_tables_t data, string_t token_kind, led_handler_t);

/* Add a left binding power to the Pratt parsing. */
void add_lbp (pratt_tables_t data, string_t token_kind,
              double binding_power);

#endif /* __LIBHAHA__PRATT_H__INCLUDED__ */

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
