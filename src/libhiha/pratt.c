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
#include <libhiha/token_t.h>
#include <libhiha/load_plugin.h>
#include <libhiha/initialize_once.h>
#include <libhiha/persistent_avl.h>
#include <libhiha/persistent_hash_map.h>
#include <libhiha/spinlock.h>
#include <libhiha/pratt.h>

#define _(msgid) HIHA_GETTEXT (msgid)

#define UNEXPECTED_TEXT _("unexpected text at %s: “%s”")

struct _pratt_map_entry
{
  string_t token_kind;
  string_t token_value;
  union
  {
    nud_handler_t nud_handler;
    led_handler_t led_handler;
    double binding_power;
  } u;
};

static bool
_pratt_equals (const struct _pratt_map_entry *key,
               const struct _pratt_map_entry *stored)
{
  bool equals =
    (string_t_cmp (key->token_kind, stored->token_kind) == 0);
  if (equals && string_t_cmp (key->token_kind, string_t_OP ()) == 0)
    /* The token is for an operator. The token_value fields must also
       be equal. */
    equals =
      (string_t_cmp (key->token_value, stored->token_value) == 0);
  return equals;
}

static token_t_hash_context_t
_pratt_init (const struct _pratt_map_entry *key)
{
  token_t t;
  int cmp = (string_t_cmp (key->token_kind, string_t_OP ()) == 0);
  if (cmp == 0)
    /* The token is for an operator. The token_value must be hashed as
       well. (It MUST be hashed, because the map never switches to a
       search method other than hashing.) */
    t = make_token_t (key->token_kind, key->token_value, NULL);
  else
    /* Any string as token_value, as long as it is the same for all
       tokens of a particular token_kind. */
    t = make_token_t (t->token_kind, empty_string_t (), NULL);
  return token_t_hash_init (t);
}

static bool
_pratt_bit (token_t_hash_context_t context, unsigned int i)
{
  const unsigned int j = i / 64;
  const unsigned int k = i % 64;
  const uint64_t hash = token_t_hash (context, j);
  const uint64_t mask = ((uint64_t) 1) << k;
  return ((hash & mask) != 0);
}

HIHA_HASH_MAP_NODES_DECL (_pratt_map, struct _pratt_map_entry);
HIHA_HASH_MAP_SEARCH_DEFN (_pratt_map_search, _pratt_map,
                           struct _pratt_map_entry, _pratt_init,
                           _pratt_bit, _pratt_equals);
HIHA_HASH_MAP_INSERT_DEFN (_pratt_map_insert, _pratt_map,
                           struct _pratt_map_entry, _pratt_init,
                           _pratt_bit, _pratt_equals);

struct pratt_tables
{
  _pratt_map_t nud;
  _pratt_map_t led;
  _pratt_map_t lbp;
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
  struct _pratt_map_entry element = {
    .token_kind = token_kind,
    .token_value = empty_string_t (),
    .u.nud_handler = handler
  };
  _pratt_map_insert (data->nud, &element,
                     hiha_hash_map_insert_or_replace, &data->nud, NULL);
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
  struct _pratt_map_entry element = {
    .token_kind = token_kind,
    .token_value = empty_string_t (),
    .u.led_handler = handler
  };
  _pratt_map_insert (data->led, &element,
                     hiha_hash_map_insert_or_replace, &data->led, NULL);
}

HIHA_VISIBLE void
pratt_lbp_put (pratt_tables_t data, string_t token_kind,
               double binding_power)
{
  struct _pratt_map_entry element = {
    .token_kind = token_kind,
    .token_value = empty_string_t (),
    .u.binding_power = binding_power
  };
  _pratt_map_insert (data->lbp, &element,
                     hiha_hash_map_insert_or_replace, &data->lbp, NULL);
}

HIHA_VISIBLE nud_handler_t
pratt_nud_get (pratt_tables_t data, string_t token_kind)
{
  nud_handler_t handler = data->nud_default;
  struct _pratt_map_entry element = {
    .token_kind = token_kind,
    .token_value = empty_string_t ()
  };
  const struct _pratt_map_entry *entry =
    _pratt_map_search (data->nud, &element);
  if (entry != NULL)
    handler = entry->u.nud_handler;
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
  struct _pratt_map_entry element = {
    .token_kind = token_kind,
    .token_value = empty_string_t ()
  };
  const struct _pratt_map_entry *entry =
    _pratt_map_search (data->led, &element);
  return (entry != NULL) ? entry->u.led_handler : NULL;
}

HIHA_VISIBLE double
pratt_lbp_get (pratt_tables_t data, string_t token_kind)
{
  struct _pratt_map_entry element = {
    .token_kind = token_kind,
    .token_value = empty_string_t ()
  };
  const struct _pratt_map_entry *entry =
    _pratt_map_search (data->lbp, &element);
  return (entry != NULL) ? entry->u.binding_power : -HUGE_VAL;
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
