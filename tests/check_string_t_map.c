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

#define HOW_MANY 10000

static int
int_identity (int n)
{
  return n;
}

static int
int_negate (int n)
{
  return -n;
}

static int
int_negate_if_even (int n)
{
  return ((n % 2) == 0) ? -n : n;
}

static bool
int_is_anything (int n)
{
  return true;
}

static bool
int_is_odd (int n)
{
  return ((n % 2) != 0);
}

static bool
int_is_even (int n)
{
  return ((n % 2) == 0);
}

static string_t_map_t
insert_into_map (string_t_map_t map, int (*f) (int))
{
  string_t_map_t result = map;
  for (int i = 1; i != HOW_MANY + 1; i += 1)
    {
      char buf[100];
      snprintf (buf, 100, "key%06d", i);
      string_t key = make_string_t (buf);
      int *value = XMALLOC (int);
      *value = f (i);
      result = string_t_map_insert_only (result, key, value);
    }
  return result;
}

static string_t_map_t
replace_in_map (string_t_map_t map, int (*f) (int))
{
  string_t_map_t result = map;
  for (int i = 1; i != HOW_MANY + 1; i += 1)
    {
      char buf[100];
      snprintf (buf, 100, "key%06d", i);
      string_t key = make_string_t (buf);
      int *value = XMALLOC (int);
      *value = f (i);
      result = string_t_map_replace_only (result, key, value);
    }
  return result;
}

static string_t_map_t
insert_into_or_replace_in_map (string_t_map_t map, int (*f) (int))
{
  string_t_map_t result = map;
  for (int i = 1; i != HOW_MANY + 1; i += 1)
    {
      char buf[100];
      snprintf (buf, 100, "key%06d", i);
      string_t key = make_string_t (buf);
      int *value = XMALLOC (int);
      *value = f (i);
      result = string_t_map_insert_or_replace (result, key, value);
    }
  return result;
}

static string_t_map_t
delete_from_map (string_t_map_t map, bool (*pred) (int))
{
  string_t_map_t result = map;
  for (int i = 1; i != HOW_MANY + 1; i += 1)
    if (pred (i))
      {
        char buf[100];
        snprintf (buf, 100, "key%06d", i);
        string_t key = make_string_t (buf);
        result = string_t_map_delete (result, key);
      }
  return result;
}

static void
check_map (string_t_map_t map, bool (*pred) (int), int (*f) (int))
{
  for (int i = 1; i != HOW_MANY + 1; i += 1)
    if (pred (i))
      {
        char buf[100];
        snprintf (buf, 100, "key%06d", i);
        string_t key = make_string_t (buf);
        const void *value = string_t_map_search (map, key);
        assert (value != NULL);
        assert (*((const int *) value) == f (i));
      }
  for (int i = 1; i != HOW_MANY + 1; i += 1)
    {
      char buf[100];
      snprintf (buf, 100, "no-such-key-%06d", i);
      string_t key = make_string_t (buf);
      const void *value = string_t_map_search (map, key);
      assert (value == NULL);
    }
}

static void
check_missing (string_t_map_t map, bool (*pred) (int))
{
  for (int i = 1; i != HOW_MANY + 1; i += 1)
    if (pred (i))
      {
        char buf[100];
        snprintf (buf, 100, "key%06d", i);
        string_t key = make_string_t (buf);
        const void *value = string_t_map_search (map, key);
        assert (value == NULL);
      }
}

int
main (void)
{
  GC_INIT ();

  assert ((HOW_MANY % 2) == 0);

  string_t_map_t map = NULL;
  assert (string_t_map_size (map) == 0);

  map = insert_into_map (map, &int_identity);
  assert (string_t_map_size (map) == HOW_MANY);
  check_map (map, &int_is_anything, &int_identity);
  map = insert_into_map (map, &int_negate);
  assert (string_t_map_size (map) == HOW_MANY);
  check_map (map, &int_is_anything, &int_identity);
  map = replace_in_map (map, &int_negate);
  assert (string_t_map_size (map) == HOW_MANY);
  check_map (map, &int_is_anything, &int_negate);
  map = insert_into_or_replace_in_map (map, &int_identity);
  assert (string_t_map_size (map) == HOW_MANY);
  check_map (map, &int_is_anything, &int_identity);

  string_t_map_t map_even = delete_from_map (map, &int_is_odd);
  assert (2 * string_t_map_size (map_even) == HOW_MANY);
  assert (string_t_map_size (map) == HOW_MANY);
  check_map (map_even, &int_is_even, &int_identity);
  check_missing (map_even, &int_is_odd);

  string_t_map_t map_odd = delete_from_map (map, &int_is_even);
  assert (2 * string_t_map_size (map_odd) == HOW_MANY);
  assert (string_t_map_size (map) == HOW_MANY);
  check_map (map_odd, &int_is_odd, &int_identity);
  check_missing (map_odd, &int_is_even);

  string_t_map_t map_empty = delete_from_map (map, &int_is_anything);
  assert (string_t_map_size (map_empty) == 0);
  assert (map_empty == NULL);

  string_t_map_t map2 = insert_into_map (map_odd, &int_negate);
  assert (string_t_map_size (map2) == HOW_MANY);
  check_map (map2, &int_is_anything, &int_negate_if_even);

  string_t_map_t map3 =
    insert_into_or_replace_in_map (map_odd, &int_negate);
  assert (string_t_map_size (map3) == HOW_MANY);
  check_map (map3, &int_is_anything, &int_negate);

  string_t_map_t map4 = replace_in_map (map_odd, &int_negate);
  assert (2 * string_t_map_size (map4) == HOW_MANY);
  check_map (map4, &int_is_odd, &int_negate);
  check_missing (map4, &int_is_even);

  string_t_vector_t vec4k = string_t_map_keys (map4);
  string_t_map_t map4a = map4;
  for (size_t i = 0; i != string_t_vector_length (vec4k); i += 1)
    map4a = string_t_map_delete (map4a, string_t_vector_ref (vec4k, i));
  assert (map4a == NULL);

  voidp_vector_t vec4v = string_t_map_values (map4);
  for (size_t i = 0; i != string_t_vector_length (vec4k); i += 1)
    {
      string_t key = string_t_vector_ref (vec4k, i);
      const void *value = string_t_map_search (map4, key);
      const size_t n = voidp_vector_length (vec4v);
      size_t j = 0;
      while (j != n && (*((const int *) voidp_vector_ref (vec4v, j))
                        != *((const int *) value)))
        j += 1;
      assert (j != n);
      const void **buf = XNMALLOC (n - 1, const void *);
      voidp_vector_refs (vec4v, 0, j, buf);
      voidp_vector_refs (vec4v, j + 1, n - j - 1, buf + j);
      vec4v = voidp_vector_pushes (NULL, n - 1, buf);
    }
  assert (vec4v == NULL);

  string_t_keyval_vector_t vec4kv = string_t_map_associations (map4);
  for (size_t i = 0; i != string_t_keyval_vector_length (vec4kv);
       i += 1)
    {
      string_t key = string_t_keyval_vector_ref (vec4kv, i)->key;
      const void *value = string_t_keyval_vector_ref (vec4kv, i)->value;
      int val1 = *((const int *) value);
      int val2 = *((const int *) string_t_map_search (map4, key));
      assert (val1 == val2);
    }

  return 0;
}
