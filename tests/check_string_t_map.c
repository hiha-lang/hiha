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
check_map (string_t_map_t map, int (*f) (int))
{
  for (int i = 1; i != HOW_MANY + 1; i += 1)
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
      snprintf (buf, 100, "yek%06d", i);
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
  check_map (map, &int_identity);
  map = insert_into_map (map, &int_negate);
  assert (string_t_map_size (map) == HOW_MANY);
  check_map (map, &int_identity);
  map = replace_in_map (map, &int_negate);
  assert (string_t_map_size (map) == HOW_MANY);
  check_map (map, &int_negate);
  map = insert_into_or_replace_in_map (map, &int_identity);
  assert (string_t_map_size (map) == HOW_MANY);
  check_map (map, &int_identity);

  string_t_map_t map_even = delete_from_map (map, &int_is_odd);
  assert (2 * string_t_map_size (map_even) == HOW_MANY);
  assert (string_t_map_size (map) == HOW_MANY);
  string_t_map_t map_odd = delete_from_map (map, &int_is_even);
  assert (2 * string_t_map_size (map_odd) == HOW_MANY);

  return 0;
}
