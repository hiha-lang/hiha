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

static string_t_set_t
adjoin_interval_contents (string_t_set_t set, bool (*pred) (int),
                          int i1, int i2)
{
  string_t_set_t result = set;
  for (int i = i1; i != i2 + 1; i += 1)
    if (pred (i))
      {
        char buf[100];
        snprintf (buf, 100, "elem%06d", i);
        string_t elem = make_string_t (buf);
        result = string_t_set_adjoin (result, elem);
      }
  return result;
}

int
main (void)
{
  GC_INIT ();

  assert ((HOW_MANY % 2) == 0);

  string_t_set_t set = NULL;
  assert (string_t_set_size (set) == 0);

  string_t_set_t set1 =
    adjoin_interval_contents (set, int_is_anything, 1, HOW_MANY);
  assert (string_t_set_size (set1) == HOW_MANY);
  string_t_set_t set2 =
    adjoin_interval_contents (set, int_is_odd, 1, HOW_MANY);
  assert (2 * string_t_set_size (set2) == HOW_MANY);
  string_t_set_t set3 =
    adjoin_interval_contents (set, int_is_even, 1, HOW_MANY);
  assert (2 * string_t_set_size (set3) == HOW_MANY);
  assert (!string_t_set_equal (set2, set3, string_t_set_end));

  string_t_set_t union_2_3 =
    string_t_set_union (set2, set3, string_t_set_end);
  assert (string_t_set_size (union_2_3) == HOW_MANY);
  assert (string_t_set_equal (set1, union_2_3, string_t_set_end));
  string_t_set_t intersection_2_3 =
    string_t_set_intersection (set2, set3, string_t_set_end);
  assert (string_t_set_size (intersection_2_3) == 0);
  assert (string_t_set_equal
          (NULL, intersection_2_3, string_t_set_end));
  string_t_set_t difference_2_3 =
    string_t_set_difference (set2, set3, string_t_set_end);
  assert (2 * string_t_set_size (difference_2_3) == HOW_MANY);
  assert (string_t_set_equal (set2, difference_2_3, string_t_set_end));

  return 0;
}
