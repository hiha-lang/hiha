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

#include <math.h>
#include <float.h>
#include <libhiha/string_t.h>
#include <libhiha/token_t.h>

inline double
minimum_binding_power (void)
{
  return -HUGE_VAL;
}

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

struct pratt_tables_entry
{
  const char *pass;
  pratt_tables_t tables;
};
typedef struct pratt_tables_entry pratt_tables_entry_t;

DECLARE_HIHA_PERSISTENT_VECTOR_DATATYPE (, pratt_tables_vector,
                                         pratt_tables_entry_t, 5);

pratt_tables_t make_pratt_tables_t (void);

void acquire_pratt_tables_lock (void);
void release_pratt_tables_lock (void);
pratt_tables_t get_pratt_tables_for_pass (const char *pass);
void set_pratt_tables_for_pass (const char *pass,
                                pratt_tables_t tables);
pratt_tables_vector_t get_pratt_tables (void);

/* The ‘state’ argument is for whatever use the programmer wishes. */
void pratt_parse (void *state, buffered_token_getter_t getter,
                  pratt_tables_t tables, double min_power,
                  token_t *lhs, const char **error_message);

typedef void nud_handler (void *state, buffered_token_getter_t getter,
                          pratt_tables_t tables, token_t tok,
                          token_t *lhs, const char **error_message);
typedef nud_handler *nud_handler_t;

typedef void led_handler (void *state, buffered_token_getter_t getter,
                          pratt_tables_t tables, token_t tok,
                          token_t *lhs, const char **error_message);
typedef led_handler *led_handler_t;

/* Add a null denotation handler, or replace an existing one. */
void pratt_nud_put (pratt_tables_t tables, string_t token_kind,
                    string_t token_value, nud_handler_t);

/* Set the default null denotation handler for a set of tables. (If
   you do not use this function to set the default, then the preset
   default simply passes the token through.) */
void pratt_nud_put_default (pratt_tables_t data, nud_handler_t);

/* Add a left denotation handler, or replace an existing one. */
void pratt_led_put (pratt_tables_t tables, string_t token_kind,
                    string_t token_value, led_handler_t);

/* Add a left binding power, or replace an existing one. */
void pratt_lbp_put (pratt_tables_t tables, string_t token_kind,
                    string_t token_value, double binding_power);

/* Get a null denotation handler from the Pratt tables, or get the
   default handler that goes along with this set of tables. Usually
   this just passes the token through. */
nud_handler_t pratt_nud_get (pratt_tables_t tables,
                             string_t token_kind, string_t token_value);

/* Get the default null denotation handler for a set of tables. */
nud_handler_t pratt_nud_get_default (pratt_tables_t data);

/* Get a left denotation handler from the Pratt tables, or return
   NULL. */
led_handler_t pratt_led_get (pratt_tables_t tables,
                             string_t token_kind, string_t token_value);

/* Get a left binding power from the Pratt tables, or return
   -HUGE_VAL. */
double pratt_lbp_get (pratt_tables_t tables, string_t token_kind,
                      string_t token_value);

#endif /* __LIBHAHA__PRATT_H__INCLUDED__ */

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
