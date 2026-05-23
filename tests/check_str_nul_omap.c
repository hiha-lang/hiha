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

static str_nul_omap_t
insert_into_omap (str_nul_omap_t omap, int (*f) (int))
{
  str_nul_omap_t result = omap;
  for (int i = 1; i != HOW_MANY + 1; i += 1)
    {
      char buf[100];
      snprintf (buf, 100, "key%06d", i);
      const char *key = xstrdup (buf);
      int *value = XMALLOC (int);
      *value = f (i);
      result = str_nul_omap_insert_only (result, key, value);
    }
  return result;
}

static str_nul_omap_t
replace_in_omap (str_nul_omap_t omap, int (*f) (int))
{
  str_nul_omap_t result = omap;
  for (int i = 1; i != HOW_MANY + 1; i += 1)
    {
      char buf[100];
      snprintf (buf, 100, "key%06d", i);
      const char *key = xstrdup (buf);
      int *value = XMALLOC (int);
      *value = f (i);
      result = str_nul_omap_replace_only (result, key, value);
    }
  return result;
}

static str_nul_omap_t
insert_into_or_replace_in_omap (str_nul_omap_t omap, int (*f) (int))
{
  str_nul_omap_t result = omap;
  for (int i = 1; i != HOW_MANY + 1; i += 1)
    {
      char buf[100];
      snprintf (buf, 100, "key%06d", i);
      const char *key = xstrdup (buf);
      int *value = XMALLOC (int);
      *value = f (i);
      result = str_nul_omap_insert_or_replace (result, key, value);
    }
  return result;
}

static str_nul_omap_t
delete_from_omap (str_nul_omap_t omap, bool (*pred) (int))
{
  str_nul_omap_t result = omap;
  for (int i = 1; i != HOW_MANY + 1; i += 1)
    if (pred (i))
      {
        char buf[100];
        snprintf (buf, 100, "key%06d", i);
        const char *key = xstrdup (buf);
        result = str_nul_omap_delete (result, key);
      }
  return result;
}

static void
check_omap (str_nul_omap_t omap, bool (*pred) (int), int (*f) (int))
{
  for (int i = 1; i != HOW_MANY + 1; i += 1)
    if (pred (i))
      {
        char buf[100];
        snprintf (buf, 100, "key%06d", i);
        const char *key = xstrdup (buf);
        const void *value = str_nul_omap_search (omap, key);
        assert (value != NULL);
        assert (*((const int *) value) == f (i));
      }
  for (int i = 1; i != HOW_MANY + 1; i += 1)
    {
      char buf[100];
      snprintf (buf, 100, "no-such-key-%06d", i);
      const char *key = xstrdup (buf);
      const void *value = str_nul_omap_search (omap, key);
      assert (value == NULL);
    }
}

static void
check_missing (str_nul_omap_t omap, bool (*pred) (int))
{
  for (int i = 1; i != HOW_MANY + 1; i += 1)
    if (pred (i))
      {
        char buf[100];
        snprintf (buf, 100, "key%06d", i);
        const char *key = xstrdup (buf);
        const void *value = str_nul_omap_search (omap, key);
        assert (value == NULL);
      }
}

int
main (void)
{
  GC_INIT ();

  assert ((HOW_MANY % 2) == 0);

  str_nul_omap_t omap = NULL;
  assert (str_nul_omap_size (omap) == 0);

  omap = insert_into_omap (omap, &int_identity);
  assert (str_nul_omap_size (omap) == HOW_MANY);
  check_omap (omap, &int_is_anything, &int_identity);
  omap = insert_into_omap (omap, &int_negate);
  assert (str_nul_omap_size (omap) == HOW_MANY);
  check_omap (omap, &int_is_anything, &int_identity);
  omap = replace_in_omap (omap, &int_negate);
  assert (str_nul_omap_size (omap) == HOW_MANY);
  check_omap (omap, &int_is_anything, &int_negate);
  omap = insert_into_or_replace_in_omap (omap, &int_identity);
  assert (str_nul_omap_size (omap) == HOW_MANY);
  check_omap (omap, &int_is_anything, &int_identity);

  str_nul_omap_t omap_even = delete_from_omap (omap, &int_is_odd);
  assert (2 * str_nul_omap_size (omap_even) == HOW_MANY);
  assert (str_nul_omap_size (omap) == HOW_MANY);
  check_omap (omap_even, &int_is_even, &int_identity);
  check_missing (omap_even, &int_is_odd);

  str_nul_omap_t omap_odd = delete_from_omap (omap, &int_is_even);
  assert (2 * str_nul_omap_size (omap_odd) == HOW_MANY);
  assert (str_nul_omap_size (omap) == HOW_MANY);
  check_omap (omap_odd, &int_is_odd, &int_identity);
  check_missing (omap_odd, &int_is_even);

  str_nul_omap_t omap_empty = delete_from_omap (omap, &int_is_anything);
  assert (str_nul_omap_size (omap_empty) == 0);
  assert (omap_empty == NULL);

  str_nul_omap_t omap2 = insert_into_omap (omap_odd, &int_negate);
  assert (str_nul_omap_size (omap2) == HOW_MANY);
  check_omap (omap2, &int_is_anything, &int_negate_if_even);

  str_nul_omap_t omap3 =
    insert_into_or_replace_in_omap (omap_odd, &int_negate);
  assert (str_nul_omap_size (omap3) == HOW_MANY);
  check_omap (omap3, &int_is_anything, &int_negate);

  str_nul_omap_t omap4 = replace_in_omap (omap_odd, &int_negate);
  assert (2 * str_nul_omap_size (omap4) == HOW_MANY);
  check_omap (omap4, &int_is_odd, &int_negate);
  check_missing (omap4, &int_is_even);

  str_nul_vector_t vec4k = str_nul_omap_keys (omap4, -1);
  //for (size_t i = 0; i != str_nul_vector_length (vec4k); i += 1)
  //  printf ("%s\n", xstrdup (str_nul_vector_ref (vec4k, i)));
  str_nul_omap_t omap4a = omap4;
  for (size_t i = 0; i != str_nul_vector_length (vec4k); i += 1)
    omap4a =
      str_nul_omap_delete (omap4a, str_nul_vector_ref (vec4k, i));
  assert (omap4a == NULL);

  voidp_vector_t vec4v = str_nul_omap_values (omap4, 1);
  for (size_t i = 0; i != str_nul_vector_length (vec4k); i += 1)
    {
      const char *key = str_nul_vector_ref (vec4k, i);
      const void *value = str_nul_omap_search (omap4, key);
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

  str_nul_keyval_vector_t vec4kv =
    str_nul_omap_associations (omap4, -1);
  for (size_t i = 0; i != str_nul_keyval_vector_length (vec4kv); i += 1)
    {
      const char *key = str_nul_keyval_vector_ref (vec4kv, i)->key;
      const void *value = str_nul_keyval_vector_ref (vec4kv, i)->value;
      int val1 = *((const int *) value);
      int val2 = *((const int *) str_nul_omap_search (omap4, key));
      assert (val1 == val2);
    }

  return 0;
}
