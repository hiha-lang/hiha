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
#include <assert.h>
#include <gc/gc.h>
#include <libhiha/libhiha.h>

HIHA_VISIBLE const char version_etc_copyright[] =
  "Copyright %s %d Barry Schwartz";

struct string2int
{
  string_t str;
  int i;
};

string_t_hash_context_t
string2int_hash_init (const struct string2int *element)
{
  return string_t_hash_init (element->str);
}

static bool
string2int_hash_bit (string_t_hash_context_t context, unsigned int i)
{
  const unsigned int j = i / 64;
  const unsigned int k = i % 64;
  const uint64_t hash = string_t_hash (context, j);
  const uint64_t mask = ((uint64_t) 1) << k;
  return ((hash & mask) != 0);
}

static bool
string2int_equals (const struct string2int *key,
                   const struct string2int *stored)
{
  return (string_t_cmp (key->str, stored->str) == 0);
}

HIHA_HASH_MAP_NODES_DECL (string2int_hash_map, struct string2int);
HIHA_HASH_MAP_SEARCH_DEFN (string2int_hash_map_search,
                           string2int_hash_map, struct string2int,
                           string2int_hash_init, string2int_hash_bit,
                           string2int_equals);
HIHA_HASH_MAP_INSERT_DEFN (string2int_hash_map_insert,
                           string2int_hash_map, struct string2int,
                           string2int_hash_init, string2int_hash_bit,
                           string2int_equals);
HIHA_HASH_MAP_DELETE_DEFN (string2int_hash_map_delete,
                           string2int_hash_map, struct string2int,
                           string2int_hash_init, string2int_hash_bit,
                           string2int_equals);
HIHA_HASH_MAP_WALK_DEFN (string2int_hash_map_walk,
                         string2int_hash_map, struct string2int);

static void
string2int_callback (const struct string2int *element, void *data)
{
  int *int_array = data;
  int n = int_array[0];
  int *buf = int_array + 1;
  int j = 0;
  while (j != n && element->i != buf[j])
    j += 1;
  assert (j != n);
  buf[j] = 9999999;
}

int
main (void)
{
  GC_INIT ();

  string2int_hash_map_t hm = NULL;

  struct string2int anything = {
    .str = make_string_t ("anything")
  };
  assert (string2int_hash_map_search (hm, &anything) == NULL);

  const int how_many = 10000;
  size_t size = 0;
  for (int i = 1; i != how_many + 1; i += 1)
    {
      char buf[100];
      snprintf (buf, 100, "elem%06d", i);
      struct string2int elem = {
        .str = make_string_t (buf),
        .i = i
      };
      int size_change;
      string2int_hash_map_insert
        (hm, &elem, hiha_hash_map_insert_or_replace, &hm, &size_change);
      size += size_change;
    }
  assert (size == how_many);
  assert (string2int_hash_map_search (hm, &anything) == NULL);
  for (int i = 1; i != how_many + 1; i += 1)
    {
      char buf[100];
      snprintf (buf, 100, "elem%06d", i);
      struct string2int keyNNNNNN = {
        .str = make_string_t (buf)
      };
      assert (string2int_hash_map_search (hm, &keyNNNNNN) != NULL);
      assert (string2int_hash_map_search (hm, &keyNNNNNN)->i == i);
    };
  for (int i = how_many; i != 0; i -= 1)
    {
      char buf[100];
      snprintf (buf, 100, "elem%06d", i);
      struct string2int elem = {
        .str = make_string_t (buf),
        .i = -i
      };
      int size_change;
      string2int_hash_map_insert
        (hm, &elem, hiha_hash_map_insert_or_replace, &hm, &size_change);
      size += size_change;
    }
  assert (size == how_many);
  assert (string2int_hash_map_search (hm, &anything) == NULL);
  for (int i = 1; i != how_many + 1; i += 1)
    {
      char buf[100];
      snprintf (buf, 100, "elem%06d", i);
      struct string2int keyNNNNNN = {
        .str = make_string_t (buf)
      };
      assert (string2int_hash_map_search (hm, &keyNNNNNN) != NULL);
      assert (string2int_hash_map_search (hm, &keyNNNNNN)->i == -i);
    };

  for (int i = 1; i < how_many + 1; i += 2)
    {
      char buf[100];
      snprintf (buf, 100, "elem%06d", i);
      struct string2int keyNNNNNN = {
        .str = make_string_t (buf)
      };
      int size_change;
      string2int_hash_map_delete (hm, &keyNNNNNN, &hm, &size_change);
      size += size_change;
    }
  assert (size == how_many / 2 || size == (how_many / 2) + 1);
  for (int i = 1; i != how_many + 1; i += 1)
    {
      char buf[100];
      snprintf (buf, 100, "elem%06d", i);
      struct string2int keyNNNNNN = {
        .str = make_string_t (buf)
      };
      if (i % 2 != 0)
        assert (string2int_hash_map_search (hm, &keyNNNNNN) == NULL);
      else
        assert (string2int_hash_map_search (hm, &keyNNNNNN)->i == -i);
    };

  int *int_array = XNMALLOC (how_many + 1, int);
  int_array[0] = how_many;
  for (int i = 1; i != how_many + 1; i += 1)
    int_array[i] = -i;
  string2int_hash_map_walk (hm, string2int_callback, int_array);
  for (int i = 1; i != how_many + 1; i += 1)
    if ((i % 2) == 0)
      assert (int_array[i] == 9999999);
    else
      assert (int_array[i] == -i);

  return 0;
}
