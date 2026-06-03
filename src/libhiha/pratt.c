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

  We use Pratt parsing in passes, starting from Unicode code points
  (in NFC canonical form).

*/

#include <config.h>
#include <assert.h>
#include <math.h>
#include <inttypes.h>
#include <filevercmp.h>
#include <libhiha/string_t.h>
#include <libhiha/load_plugin.h>
#include <libhiha/initialize_once.h>
#include <libhiha/persistent_avl.h>
#include <libhiha/spinlock.h>
#include <libhiha/pratt.h>

#define _(msgid) HIHA_GETTEXT (msgid)

#define UNEXPECTED_TEXT _("unexpected text at %s: “%s”")

struct pratt_tables
{
  string_t_map_t nud;
  string_t_map_t led;
  string_t_map_t lbp;
  nud_handler_t nud_default;
};
typedef struct pratt_tables *pratt_tables_t;

DEFINE_HIHA_PERSISTENT_VECTOR_DATATYPE (HIHA_VISIBLE,
                                        pratt_tables_vector,
                                        pratt_tables_entry_t, 5);

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

static void
passthrough_nud_handler (void *state, buffered_token_getter_t getter,
                         pratt_tables_t tables, token_t tok,
                         token_t *lhs, const char **error_message)
{
  if (*error_message == NULL)
    *lhs = tok;
}

HIHA_VISIBLE pratt_tables_t
make_pratt_tables_t (void)
{
  pratt_tables_t data = XMALLOC (struct pratt_tables);
  data->nud = NULL;
  data->led = NULL;
  data->lbp = NULL;
  data->nud_default = &passthrough_nud_handler;
  return data;
}

static int
pratt_tables_entry_cmp (const pratt_tables_entry_t *a,
                        const pratt_tables_entry_t *b)
{
  return filevercmp (a->pass, b->pass);
}

HIHA_AVL_NODE_DECL (_pratt_tables_map_node, pratt_tables_entry_t);
HIHA_AVL_INSERT_DEFN (_pratt_tables_map_insert, _pratt_tables_map_node,
                      pratt_tables_entry_t);
HIHA_AVL_SEARCH_DEFN (_pratt_tables_map_search, _pratt_tables_map_node,
                      pratt_tables_entry_t);
HIHA_AVL_INORDER_DEFN (_pratt_tables_map_inorder,
                       _pratt_tables_map_node);

static initialize_once_t _pratt_tables_map_init1t =
  INITIALIZE_ONCE_T_INIT;
_pratt_tables_map_node_t _pratt_tables_map;
spinlock_t _pratt_tables_map_spinlock = SPINLOCK_T_INIT;

static void
_initialize_pratt_tables_map (void)
{
  _pratt_tables_map = NULL;
}

HIHA_VISIBLE void
acquire_pratt_tables_lock (void)
{
  acquire_spinlock (&_pratt_tables_map_spinlock);
}

HIHA_VISIBLE void
release_pratt_tables_lock (void)
{
  release_spinlock (&_pratt_tables_map_spinlock);
}

HIHA_VISIBLE pratt_tables_t
get_pratt_tables_for_pass (const char *pass)
{
  pratt_tables_entry_t contents = {
    .pass = pass,
    .tables = NULL
  };
  _pratt_tables_map_node_t node =
    _pratt_tables_map_search (_pratt_tables_map, contents,
                              &pratt_tables_entry_cmp);
  if (node == NULL)
    {
      contents.tables = make_pratt_tables_t ();
      _pratt_tables_map =
        _pratt_tables_map_insert (_pratt_tables_map, contents,
                                  &pratt_tables_entry_cmp);
      node =
        _pratt_tables_map_search (_pratt_tables_map, contents,
                                  &pratt_tables_entry_cmp);
    }
  pratt_tables_t value = node->data.tables;
  return value;
}

HIHA_VISIBLE void
set_pratt_tables_for_pass (const char *pass, pratt_tables_t tables)
{
  pratt_tables_entry_t contents = {
    .pass = pass,
    .tables = tables
  };
  _pratt_tables_map =
    _pratt_tables_map_insert (_pratt_tables_map, contents,
                              &pratt_tables_entry_cmp);
}

static void
collect_pratt_tables (struct _pratt_tables_map_node *node, void *data)
{
  pratt_tables_vector_t vec = *((pratt_tables_vector_t *) data);
  vec = pratt_tables_vector_push (vec, node->data);
  *((pratt_tables_vector_t *) data) = vec;
}

HIHA_VISIBLE pratt_tables_vector_t
get_pratt_tables (void)
{
  pratt_tables_vector_t vec = NULL;
  acquire_pratt_tables_lock ();
  _pratt_tables_map_inorder (_pratt_tables_map, 1, collect_pratt_tables,
                             &vec);
  release_pratt_tables_lock ();
  return vec;
}

HIHA_VISIBLE void
pratt_nud_put (pratt_tables_t data, string_t token_kind,
               nud_handler_t handler)
{
  data->nud =
    string_t_map_insert_or_replace (data->nud, token_kind, handler);
}

HIHA_VISIBLE void
pratt_nud_put_default (pratt_tables_t data, nud_handler_t handler)
{
  data->nud_default = handler;
}

HIHA_VISIBLE void
pratt_led_put (pratt_tables_t data, string_t token_kind,
               led_handler_t handler)
{
  data->led =
    string_t_map_insert_or_replace (data->led, token_kind, handler);
}

HIHA_VISIBLE void
pratt_lbp_put (pratt_tables_t data, string_t token_kind,
               double binding_power)
{
  double *bp = XMALLOC (double);
  *bp = binding_power;
  data->lbp =
    string_t_map_insert_or_replace (data->lbp, token_kind, bp);
}

HIHA_VISIBLE nud_handler_t
pratt_nud_get (pratt_tables_t data, string_t token_kind)
{
  nud_handler_t handler =
    (nud_handler_t) string_t_map_search (data->nud, token_kind);
  if (handler == NULL)
    handler = data->nud_default;
  return handler;
}

HIHA_VISIBLE nud_handler_t
pratt_nud_get_default (pratt_tables_t data)
{
  return data->nud_default;
}

HIHA_VISIBLE led_handler_t
pratt_led_get (pratt_tables_t data, string_t token_kind)
{
  return (led_handler_t) string_t_map_search (data->nud, token_kind);
}

HIHA_VISIBLE double
pratt_lbp_get (pratt_tables_t data, string_t token_kind)
{
  const void *p = string_t_map_search (data->lbp, token_kind);
  return (p == NULL) ? -HUGE_VAL : *((const double *) p);
}

static void
execute_null_denotation (void *state, buffered_token_getter_t getter,
                         pratt_tables_t tables, token_t *lhs,
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
      assert (handler != NULL);
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
                         pratt_tables_t tables, token_t *lhs,
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
             token_t *lhs, const char **error_message)
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
