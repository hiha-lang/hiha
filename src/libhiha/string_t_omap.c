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
#include <string.h>
#include <xalloc.h>
#include <libhiha/initialize_once.h>
#include <libhiha/string_t.h>
#include <libhiha/persistent_avl.h>

#define _(msgid) HIHA_GETTEXT (msgid)

HIHA_AVL_NODE_DECL (string_t_omap_node, struct string_t_keyval);
HIHA_AVL_SEARCH_DEFN (string_t_omap_node_search, string_t_omap_node,
                      struct string_t_keyval);
HIHA_AVL_INSERT_DEFN (string_t_omap_node_insert, string_t_omap_node,
                      struct string_t_keyval);

struct string_t_omap
{
  string_t_omap_node_t _tree;
  size_t _size;
  int (*_compare) (string_t_keyval_t, string_t_keyval_t);
};

static int
default_compare (string_t_keyval_t kv1, string_t_keyval_t kv2)
{
  return string_t_cmp (kv1->key, kv2->key);
}

static initialize_once_t _default_empty_omap_init1t =
  INITIALIZE_ONCE_T_INIT;
string_t_omap_t _default_empty_omap;

static void
_initialize_default_empty_omap (void)
{
  _default_empty_omap = string_t_omap_init (&default_compare);
}

static string_t_omap_t
default_empty_omap (void)
{
  INITIALIZE_ONCE (_default_empty_omap_init1t,
                   _initialize_default_empty_omap);
  return _default_empty_omap;
}

static inline string_t_omap_t
allocated_omap (string_t_omap_t omap)
{
  return (omap != NULL) ? omap : default_empty_omap ();
}

HIHA_VISIBLE size_t
string_t_omap_size (string_t_omap_t omap)
{
  return (omap != NULL) ? omap->_size : 0;
}

HIHA_VISIBLE const void *
string_t_omap_search (string_t_omap_t omap, string_t key)
{
  const void *retval = NULL;
  if (omap != NULL && omap->_size != 0)
    {
      struct string_t_keyval kv = {
        .key = key,
        .value = NULL
      };
      string_t_omap_node_t p =
        string_t_omap_node_search (omap->_tree, kv, omap->_compare);
      if (p != NULL)
        retval = p->data.value;
    }
  return retval;
}

HIHA_VISIBLE string_t_omap_t
string_t_omap_init (int (*compare) (string_t_keyval_t,
                                    string_t_keyval_t))
{
  struct string_t_omap *result = XMALLOC (struct string_t_omap);
  result->_tree = NULL;
  result->_size = 0;
  result->_compare = compare;
  return result;
}

HIHA_VISIBLE string_t_omap_t
string_t_omap_insert_or_replace (string_t_omap_t omap,
                                 string_t key, const void *value)
{
  string_t_omap_t om = allocated_omap (omap);
  struct string_t_keyval kv = {
    .key = key,
    .value = value
  };
  const void *p =
    string_t_omap_node_search (om->_tree, kv, om->_compare);
  ssize_t size_change = (p == NULL) ? 1 : 0;
  struct string_t_omap *result = XMALLOC (struct string_t_omap);
  result->_tree =
    string_t_omap_node_insert (om->_tree, kv, om->_compare);
  result->_size = om->_size + size_change;
  result->_compare = om->_compare;
  return result;
}

HIHA_VISIBLE string_t_omap_t
string_t_omap_insert_only (string_t_omap_t omap,
                           string_t key, const void *value)
{
  string_t_omap_t om = allocated_omap (omap);
  struct string_t_keyval kv = {
    .key = key,
    .value = value
  };
  const void *p =
    string_t_omap_node_search (om->_tree, kv, om->_compare);
  string_t_omap_t retval;
  if (p != NULL)
    retval = omap;
  else
    {
      struct string_t_omap *result = XMALLOC (struct string_t_omap);
      result->_tree =
        string_t_omap_node_insert (om->_tree, kv, om->_compare);
      result->_size = om->_size + 1;
      result->_compare = om->_compare;
      retval = result;
    }
  return retval;
}

HIHA_VISIBLE string_t_omap_t
string_t_omap_replace_only (string_t_omap_t omap,
                            string_t key, const void *value)
{
  string_t_omap_t retval;
  if (omap == NULL || omap->_size == 0)
    retval = omap;
  else
    {
      struct string_t_keyval kv = {
        .key = key,
        .value = value
      };
      const void *p =
        string_t_omap_node_search (omap->_tree, kv, omap->_compare);
      if (p == NULL)
        retval = omap;
      else
        {
          struct string_t_omap *result = XMALLOC (struct string_t_omap);
          result->_tree =
            string_t_omap_node_insert (omap->_tree, kv, omap->_compare);
          result->_size = omap->_size;
          result->_compare = omap->_compare;
          retval = result;
        }
    }
  return retval;
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
