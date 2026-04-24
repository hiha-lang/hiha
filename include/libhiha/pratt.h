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

struct pratt_handler_reference
{
  size_t register_no;           /* Register no. of a plugin. */
  size_t handler_no;            /* Handler no. within the plugin. */
};
typedef struct pratt_handler_reference *pratt_handler_reference_t;

pratt_handler_reference_t
make_pratt_handler_reference_t (size_t register_no, size_t handler_no);

struct pratt_tables;
typedef struct pratt_tables *pratt_tables_t;

pratt_tables_t make_pratt_tables_t (void);

/* The ‘state’ argument is for whatever use the programmer wishes. The
   ‘*lhs’ argument would typically represent token_t for lexical
   analysis and a parse tree for syntactic analysis. */
void pratt_parse (void *state, buffered_token_getter_t getter,
                  pratt_tables_t tables, double min_power,
                  void **lhs, const char **error_message);

typedef void nud_handler (void *state, pratt_tables_t tables,
                          token_t tok, void **lhs,
                          const char **error_message);
typedef nud_handler *nud_handler_t;

typedef void led_handler (void *state, pratt_tables_t tables,
                          token_t tok, void **lhs,
                          const char **error_message);
typedef led_handler *led_handler_t;

/* Add a reference to a null denotation handler, or replace an
   existing one. */
void pratt_nud_put (pratt_tables_t tables, string_t token_kind,
                    pratt_handler_reference_t);

/* Add a reference to a left denotation handler, or replace an
   existing one. */
void pratt_led_put (pratt_tables_t tables, string_t token_kind,
                    pratt_handler_reference_t);

/* Add a left binding power, or replace an existing one. */
void pratt_lbp_put (pratt_tables_t tables, string_t token_kind,
                    double binding_power);

/* Get a null denotation handler from the Pratt tables, or return
   NULL. */
nud_handler_t pratt_nud_handler_get (pratt_tables_t tables,
                                     string_t token_kind);

/* Get a left denotation handler from the Pratt tables, or return
   NULL. */
led_handler_t pratt_led_handler_get (pratt_tables_t tables,
                                     string_t token_kind);

/* Get a left binding power from the Pratt tables, or return
   -HUGE_VAL. */
double pratt_lbp_get (pratt_tables_t tables, string_t token_kind);

#endif /* __LIBHAHA__PRATT_H__INCLUDED__ */

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
