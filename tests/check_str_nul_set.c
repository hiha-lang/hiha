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

static str_nul_set_t
adjoin_interval_contents (str_nul_set_t set, bool (*pred) (int),
                          int i1, int i2)
{
  str_nul_set_t result = set;
  for (int i = i1; i != i2 + 1; i += 1)
    if (pred (i))
      {
        char buf[100];
        snprintf (buf, 100, "elem%06d", i);
        const char *elem = xstrdup (buf);
        result = str_nul_set_adjoin (result, elem);
      }
  return result;
}

int
main (void)
{
  GC_INIT ();

  assert ((HOW_MANY % 2) == 0);

  str_nul_set_t set;
  str_nul_set_t set1;
  str_nul_set_t set2;
  str_nul_set_t set3;
  str_nul_set_t set4;

  set = NULL;
  assert (str_nul_set_size (set) == 0);

  set1 = adjoin_interval_contents (set, int_is_anything, 1, HOW_MANY);
  assert (str_nul_set_size (set1) == HOW_MANY);
  set2 = adjoin_interval_contents (set, int_is_odd, 1, HOW_MANY);
  assert (2 * str_nul_set_size (set2) == HOW_MANY);
  set3 = adjoin_interval_contents (set, int_is_even, 1, HOW_MANY);
  assert (2 * str_nul_set_size (set3) == HOW_MANY);
  assert (!str_nul_set_equal (set2, set3, str_nul_set_end));

  str_nul_set_t union_2_3 =
    str_nul_set_union (set2, set3, str_nul_set_end);
  assert (str_nul_set_size (union_2_3) == HOW_MANY);
  assert (str_nul_set_equal (set1, union_2_3, str_nul_set_end));
  str_nul_set_t intersection_2_3 =
    str_nul_set_intersection (set2, set3, str_nul_set_end);
  assert (str_nul_set_size (intersection_2_3) == 0);
  assert (str_nul_set_equal (NULL, intersection_2_3, str_nul_set_end));
  str_nul_set_t difference_2_3 =
    str_nul_set_difference (set2, set3, str_nul_set_end);
  assert (2 * str_nul_set_size (difference_2_3) == HOW_MANY);
  assert (str_nul_set_equal (set2, difference_2_3, str_nul_set_end));

  assert (str_nul_set_equal
          (str_nul_set_intersection (adjoin_interval_contents
                                     (NULL, int_is_anything, 1, 70),
                                     adjoin_interval_contents
                                     (NULL, int_is_anything, 51, 100),
                                     adjoin_interval_contents
                                     (NULL, int_is_anything, -99, 200),
                                     str_nul_set_end),
           str_nul_set_union (adjoin_interval_contents
                              (NULL, int_is_anything, 51, 55),
                              adjoin_interval_contents
                              (NULL, int_is_anything, 53, 62),
                              adjoin_interval_contents
                              (NULL, int_is_anything, 61, 70),
                              str_nul_set_end),
           str_nul_set_difference (adjoin_interval_contents
                                   (NULL, int_is_anything, -99, 200),
                                   adjoin_interval_contents
                                   (NULL, int_is_anything, -99, 0),
                                   adjoin_interval_contents
                                   (NULL, int_is_anything, 71, 300),
                                   adjoin_interval_contents
                                   (NULL, int_is_anything, -10, 50),
                                   str_nul_set_end),
           adjoin_interval_contents (NULL, int_is_anything, 51, 70),
           str_nul_set_end));

  assert (str_nul_set_subset (NULL, NULL, NULL, NULL, str_nul_set_end));
  assert (str_nul_set_subset (NULL, NULL, NULL, NULL,
                              adjoin_interval_contents
                              (NULL, int_is_anything, 1, 10),
                              adjoin_interval_contents
                              (NULL, int_is_anything, 1, 10),
                              adjoin_interval_contents
                              (NULL, int_is_anything, 1, 20),
                              adjoin_interval_contents
                              (NULL, int_is_anything, 1, 30),
                              str_nul_set_end));
  assert (!str_nul_set_subset (NULL, NULL, NULL, NULL,
                               adjoin_interval_contents
                               (NULL, int_is_anything, 1, 10),
                               adjoin_interval_contents
                               (NULL, int_is_anything, 1, 10),
                               adjoin_interval_contents
                               (NULL, int_is_anything, 1, 20),
                               adjoin_interval_contents
                               (NULL, int_is_anything, 2, 30),
                               str_nul_set_end));

  assert (!str_nul_set_proper_subset (NULL, NULL, NULL, NULL,
                                      str_nul_set_end));
  assert (!str_nul_set_proper_subset (NULL, NULL, NULL, NULL,
                                      adjoin_interval_contents
                                      (NULL, int_is_anything, 1, 10),
                                      adjoin_interval_contents
                                      (NULL, int_is_anything, 1, 10),
                                      adjoin_interval_contents
                                      (NULL, int_is_anything, 1, 20),
                                      adjoin_interval_contents
                                      (NULL, int_is_anything, 1, 30),
                                      str_nul_set_end));
  assert (str_nul_set_proper_subset (NULL,
                                     adjoin_interval_contents
                                     (NULL, int_is_anything, 1, 10),
                                     adjoin_interval_contents
                                     (NULL, int_is_anything, 1, 20),
                                     adjoin_interval_contents
                                     (NULL, int_is_anything, 1, 30),
                                     str_nul_set_end));

  assert (str_nul_set_superset (NULL, NULL, NULL, NULL,
                                str_nul_set_end));
  assert (str_nul_set_superset (adjoin_interval_contents
                                (NULL, int_is_anything, 1, 30),
                                adjoin_interval_contents
                                (NULL, int_is_anything, 1, 20),
                                adjoin_interval_contents
                                (NULL, int_is_anything, 1, 10),
                                adjoin_interval_contents
                                (NULL, int_is_anything, 1, 10),
                                NULL, NULL, NULL, NULL,
                                str_nul_set_end));
  assert (!str_nul_set_superset (adjoin_interval_contents
                                 (NULL, int_is_anything, 2, 30),
                                 adjoin_interval_contents
                                 (NULL, int_is_anything, 1, 20),
                                 adjoin_interval_contents
                                 (NULL, int_is_anything, 1, 10),
                                 adjoin_interval_contents
                                 (NULL, int_is_anything, 1, 10),
                                 NULL, NULL, NULL, NULL,
                                 str_nul_set_end));

  assert (!str_nul_set_proper_superset (NULL, NULL, NULL, NULL,
                                        str_nul_set_end));
  assert (!str_nul_set_proper_superset (adjoin_interval_contents
                                        (NULL, int_is_anything, 1, 30),
                                        adjoin_interval_contents
                                        (NULL, int_is_anything, 1, 20),
                                        adjoin_interval_contents
                                        (NULL, int_is_anything, 1, 10),
                                        adjoin_interval_contents
                                        (NULL, int_is_anything, 1, 10),
                                        NULL, NULL, NULL, NULL,
                                        str_nul_set_end));
  assert (str_nul_set_proper_superset (adjoin_interval_contents
                                       (NULL, int_is_anything, 1, 30),
                                       adjoin_interval_contents
                                       (NULL, int_is_anything, 1, 20),
                                       adjoin_interval_contents
                                       (NULL, int_is_anything, 1, 10),
                                       NULL, str_nul_set_end));

  set1 = adjoin_interval_contents (NULL, int_is_anything, 1, 100);
  set2 = adjoin_interval_contents (NULL, int_is_anything, 21, 120);
  set3 = str_nul_set_symmetric_difference (set1, set2);
  set4 = str_nul_set_symmetric_difference (set2, set1);
  assert (str_nul_set_equal
          (set3, set4, (str_nul_set_union
                        (adjoin_interval_contents
                         (NULL, int_is_anything, 1, 20),
                         adjoin_interval_contents
                         (NULL, int_is_anything, 101, 120),
                         str_nul_set_end)), str_nul_set_end));

  return 0;
}
