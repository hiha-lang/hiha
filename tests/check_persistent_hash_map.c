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
#include <gc/gc.h>
#include <assert.h>
#include <libhiha/libhiha.h>

HIHA_VISIBLE const char version_etc_copyright[] =
  "Copyright %s %d Barry Schwartz";

struct string2int
{
  string_t str;
  int i;
};

static bool
string2int_equals (const struct string2int *key,
                   const struct string2int *stored)
{
  return (string_t_cmp (key->str, stored->str) == 0);
}

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

HIHA_HASH_MAP_NODES_DECL (string2int_hash_map, struct string2int);
HIHA_HASH_MAP_SEARCH_DEFN (string2int_hash_map_search,
                           string2int_hash_map, struct string2int,
                           string2int_hash_init, string2int_hash_bit,
                           string2int_equals);
HIHA_HASH_MAP_INSERT_DEFN (string2int_hash_map_insert,
                           string2int_hash_map, struct string2int,
                           string2int_hash_init, string2int_hash_bit,
                           string2int_equals);

int
main (void)
{
  GC_INIT ();

  string2int_hash_map_t hm = NULL;

  struct string2int key = {
    .str = make_string_t ("anything")
  };
  assert (string2int_hash_map_search (hm, &key) == NULL);

  return 0;
}
