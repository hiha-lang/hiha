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
#include <libhiha/libhiha.h>

HIHA_VISIBLE const char version_etc_copyright[] =
  "Copyright %s %d Barry Schwartz";

DECLARE_HIHA_PERSISTENT_VECTOR_DATATYPE (static, int_vector, int, 5);
DEFINE_HIHA_PERSISTENT_VECTOR_DATATYPE (static, int_vector, int, 5);

static void
shuffle (size_t n, int buf[n])
{
  for (size_t i = n - 1; i != 0; i -= 1)
    {
      /* Dividing the big random integer is good enough. */
      size_t j = (size_t) (random () % ((long) i) + 1);

      int tmp = buf[i];
      buf[i] = buf[j];
      buf[j] = tmp;
    }
}

static int
int_cmp (const int *p1, const int *p2, void *data)
{
  return (*p1 < *p2) ? -1 : ((*p1 == *p2) ? 0 : 1);
}

static void
test_small_merge (void)
{
  int buf1[4] = { 1, 5, 7, 12 };
  int buf2[6] = { 2, 3, 8, 10, 11, 13 };
  int_vector_t v1 = int_vector_pushes (NULL, 4, buf1);
  int_vector_t v2 = int_vector_pushes (NULL, 6, buf2);
  assert (int_vector_length (v1) == 4);
  assert (int_vector_length (v2) == 6);

  int_vector_t v3 = int_vector_merge (v1, 0, 4, v2, 0, 6,
                                      &int_cmp, NULL);
  assert (int_vector_length (v3) == 10);
  assert (int_vector_ref (v3, 0) == 1);
  assert (int_vector_ref (v3, 1) == 2);
  assert (int_vector_ref (v3, 2) == 3);
  assert (int_vector_ref (v3, 3) == 5);
  assert (int_vector_ref (v3, 4) == 7);
  assert (int_vector_ref (v3, 5) == 8);
  assert (int_vector_ref (v3, 6) == 10);
  assert (int_vector_ref (v3, 7) == 11);
  assert (int_vector_ref (v3, 8) == 12);
  assert (int_vector_ref (v3, 9) == 13);

  int_vector_t v4 = int_vector_merge (v2, 0, 6, v1, 0, 4,
                                      &int_cmp, NULL);
  assert (int_vector_length (v4) == 10);
  assert (int_vector_ref (v4, 0) == 1);
  assert (int_vector_ref (v4, 1) == 2);
  assert (int_vector_ref (v4, 2) == 3);
  assert (int_vector_ref (v4, 3) == 5);
  assert (int_vector_ref (v4, 4) == 7);
  assert (int_vector_ref (v4, 5) == 8);
  assert (int_vector_ref (v4, 6) == 10);
  assert (int_vector_ref (v4, 7) == 11);
  assert (int_vector_ref (v4, 8) == 12);
  assert (int_vector_ref (v4, 9) == 13);

  int buf12[10] = { 1, 5, 7, 12, 2, 3, 8, 10, 11, 13 };
  int_vector_t v12 = int_vector_pushes (NULL, 10, buf12);
  assert (int_vector_length (v12) == 10);

  v3 = int_vector_merge (v12, 0, 4, v12, 4, 10, &int_cmp, NULL);
  assert (int_vector_length (v3) == 10);
  assert (int_vector_ref (v3, 0) == 1);
  assert (int_vector_ref (v3, 1) == 2);
  assert (int_vector_ref (v3, 2) == 3);
  assert (int_vector_ref (v3, 3) == 5);
  assert (int_vector_ref (v3, 4) == 7);
  assert (int_vector_ref (v3, 5) == 8);
  assert (int_vector_ref (v3, 6) == 10);
  assert (int_vector_ref (v3, 7) == 11);
  assert (int_vector_ref (v3, 8) == 12);
  assert (int_vector_ref (v3, 9) == 13);

  v4 = int_vector_merge (v12, 4, 10, v12, 0, 4, &int_cmp, NULL);
  assert (int_vector_length (v3) == 10);
  assert (int_vector_ref (v3, 0) == 1);
  assert (int_vector_ref (v3, 1) == 2);
  assert (int_vector_ref (v3, 2) == 3);
  assert (int_vector_ref (v3, 3) == 5);
  assert (int_vector_ref (v3, 4) == 7);
  assert (int_vector_ref (v3, 5) == 8);
  assert (int_vector_ref (v3, 6) == 10);
  assert (int_vector_ref (v3, 7) == 11);
  assert (int_vector_ref (v3, 8) == 12);
  assert (int_vector_ref (v3, 9) == 13);
}

static void
test_large_merge (void)
{
  int_vector_t v1 = NULL;
  int_vector_t v2 = NULL;
  for (size_t i = 0; i != 1000; i += 2)
    v1 = int_vector_push (v1, i);
  for (size_t i = 1; i != 1001; i += 2)
    v2 = int_vector_push (v2, i);
  assert (int_vector_length (v1) == 500);
  assert (int_vector_length (v2) == 500);

  int_vector_t v3 = int_vector_merge (v1, 0, 500, v2, 0, 500,
                                      &int_cmp, NULL);
  assert (int_vector_length (v3) == 1000);
  for (size_t i = 0; i != 1000; i += 1)
    assert (int_vector_ref (v3, i) == i);

  int_vector_t v4 = int_vector_merge (v2, 0, 500, v1, 0, 500,
                                      &int_cmp, NULL);
  assert (int_vector_length (v4) == 1000);
  for (size_t i = 0; i != 1000; i += 1)
    assert (int_vector_ref (v4, i) == i);

  v1 = NULL;
  for (size_t i = 0; i != 1000; i += 2)
    v1 = int_vector_push (v1, i);
  for (size_t i = 1; i != 1001; i += 2)
    v1 = int_vector_push (v1, i);

  v3 = int_vector_merge (v1, 0, 500, v1, 500, 1000, &int_cmp, NULL);
  assert (int_vector_length (v3) == 1000);
  for (size_t i = 0; i != 1000; i += 1)
    assert (int_vector_ref (v3, i) == i);

  v4 = int_vector_merge (v1, 500, 1000, v1, 0, 500, &int_cmp, NULL);
  assert (int_vector_length (v4) == 1000);
  for (size_t i = 0; i != 1000; i += 1)
    assert (int_vector_ref (v4, i) == i);
}

static void
test_small_sort (void)
{
  int buf1[9] = { 52, 23, 38, 10, 1, 13, 12, 77, 5 };
  int_vector_t v1 = NULL;
  for (size_t i = 0; i != 9; i += 1)
    v1 = int_vector_push (v1, buf1[i]);
  assert (int_vector_length (v1) == 9);

  int_vector_t v2 = int_vector_sort (v1, 0, 9, int_cmp, NULL);
  assert (int_vector_length (v2) == 9);
  assert (int_vector_ref (v2, 0) == 1);
  assert (int_vector_ref (v2, 1) == 5);
  assert (int_vector_ref (v2, 2) == 10);
  assert (int_vector_ref (v2, 3) == 12);
  assert (int_vector_ref (v2, 4) == 13);
  assert (int_vector_ref (v2, 5) == 23);
  assert (int_vector_ref (v2, 6) == 38);
  assert (int_vector_ref (v2, 7) == 52);
  assert (int_vector_ref (v2, 8) == 77);

  int_vector_t v3 = int_vector_sort (v1, 3, 6, int_cmp, NULL);
  assert (int_vector_length (v3) == 3);
  assert (int_vector_ref (v3, 0) == 1);
  assert (int_vector_ref (v3, 1) == 10);
  assert (int_vector_ref (v3, 2) == 13);
}

static void
test_large_sort (void)
{
  const size_t n = 10000;

  int buf1[n];
  for (size_t i = 0; i != n; i += 1)
    buf1[i] = i + 1;
  shuffle (n, buf1);
  int_vector_t v1 = NULL;
  for (size_t i = 0; i != n; i += 1)
    v1 = int_vector_push (v1, buf1[i]);
  assert (int_vector_length (v1) == n);

  int_vector_t v2 = int_vector_sort (v1, 0, n, int_cmp, NULL);
  assert (int_vector_length (v2) == n);
  for (size_t i = 0; i != n; i += 1)
    assert (int_vector_ref (v2, i) == i + 1);
}

int
main (void)
{
  GC_INIT ();
  test_small_merge ();
  test_large_merge ();
  test_small_sort ();
  test_large_sort ();
  return 0;
}
