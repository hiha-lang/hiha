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

  int_vector_t v3 = int_vector_merge (v1, v2, &int_cmp, NULL);
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

  int_vector_t v4 = int_vector_merge (v2, v1, &int_cmp, NULL);
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

  int_vector_t v3 = int_vector_merge (v1, v2, &int_cmp, NULL);
  assert (int_vector_length (v3) == 1000);
  for (size_t i = 0; i != 1000; i += 1)
    assert (int_vector_ref (v3, i) == i);

  int_vector_t v4 = int_vector_merge (v2, v1, &int_cmp, NULL);
  assert (int_vector_length (v4) == 1000);
  for (size_t i = 0; i != 1000; i += 1)
    assert (int_vector_ref (v4, i) == i);
}

int
main (void)
{
  GC_INIT ();
  test_small_merge ();
  test_large_merge ();
  return 0;
}
