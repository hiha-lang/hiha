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
#include <libhiha/string_t.h>
#include <libhiha/persistent_hash_map.h>

#define _(msgid) HIHA_GETTEXT (msgid)

struct string_t_map_contents
{
  string_t key;
  const void *value;
};

string_t_hash_context_t
string_t_map_hash_init (const struct string_t_map_contents *element)
{
  return string_t_hash_init (element->key);
}

static bool
string_t_map_hash_bit (string_t_hash_context_t context, unsigned int i)
{
  const unsigned int j = i / 64;
  const unsigned int k = i % 64;
  const uint64_t hash = string_t_hash (context, j);
  const uint64_t mask = ((uint64_t) 1) << k;
  return ((hash & mask) != 0);
}

static bool
  string_t_map_contents_equals
  (const struct string_t_map_contents *key,
   const struct string_t_map_contents *stored)
{
  return (string_t_cmp (key->key, stored->key) == 0);
}

HIHA_HASH_MAP_NODES_DECL (string_t_map_node,
                          struct string_t_map_contents);
HIHA_HASH_MAP_SEARCH_DEFN (string_t_map_node_search, string_t_map_node,
                           struct string_t_map_contents,
                           string_t_map_hash_init,
                           string_t_map_hash_bit,
                           string_t_map_contents_equals);
HIHA_HASH_MAP_INSERT_DEFN (string_t_map_node_insert, string_t_map_node,
                           struct string_t_map_contents,
                           string_t_map_hash_init,
                           string_t_map_hash_bit,
                           string_t_map_contents_equals);
HIHA_HASH_MAP_DELETE_DEFN (string_t_map_node_delete, string_t_map_node,
                           struct string_t_map_contents,
                           string_t_map_hash_init,
                           string_t_map_hash_bit,
                           string_t_map_contents_equals);

struct string_t_map
{
  string_t_map_node_t _trie;
  size_t _size;
};

HIHA_VISIBLE size_t
string_t_map_size (string_t_map_t map)
{
  return (map == NULL) ? 0 : map->_size;
}

HIHA_VISIBLE const void *
string_t_map_search (string_t_map_t map, const string_t key)
{
  const void *result = NULL;
  if (map != NULL)
    {
      struct string_t_map_contents element = {
        .key = key
      };
      const struct string_t_map_contents *contents =
        string_t_map_node_search (map->_trie, &element);
      if (contents != NULL)
        result = contents->value;
    }
  return result;
}

static string_t_map_t
string_t_map_insert (string_t_map_t map, string_t key,
                     const void *value, hiha_hash_map_mode_t mode)
{
  string_t_map_t result = XZALLOC (struct string_t_map);
  if (map != NULL)
    {
      result->_trie = map->_trie;
      result->_size = map->_size;
    }
  struct string_t_map_contents element = {
    .key = key,
    .value = value
  };
  ssize_t size_change;
  string_t_map_node_insert (result->_trie, &element, mode,
                            &result->_trie, &size_change);
  result->_size += size_change;
  return result;
}

HIHA_VISIBLE string_t_map_t
string_t_map_insert_or_replace (string_t_map_t map, string_t key,
                                const void *value)
{
  return string_t_map_insert (map, key, value,
                              hiha_hash_map_insert_or_replace);
}

HIHA_VISIBLE string_t_map_t
string_t_map_insert_only (string_t_map_t map, string_t key,
                          const void *value)
{
  return string_t_map_insert (map, key, value,
                              hiha_hash_map_insert_only);
}

HIHA_VISIBLE string_t_map_t
string_t_map_replace_only (string_t_map_t map, string_t key,
                           const void *value)
{
  string_t_map_t result = map;
  if (result != NULL)
    result = string_t_map_insert (result, key, value,
                                  hiha_hash_map_replace_only);
  return result;
}

HIHA_VISIBLE string_t_map_t
string_t_map_delete (string_t_map_t map, string_t key)
{
  string_t_map_t result = NULL;
  if (map != NULL)
    {
      assert (map->_size != 0);
      struct string_t_map_contents element = {
        .key = key
      };
      string_t_map_node_t trie = map->_trie;
      size_t size = map->_size;
      ssize_t size_change;
      string_t_map_node_delete (trie, &element, &trie, &size_change);
      size += size_change;
      assert ((trie == NULL) == (size == 0));
      if (size != 0)
        {
          result = XZALLOC (struct string_t_map);
          result->_trie = trie;
          result->_size = size;
        }
    }
  return result;
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
