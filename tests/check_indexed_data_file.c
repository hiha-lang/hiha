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

#define STRINGIFY(s) #s
#define EXPANDED(s) STRINGIFY(s)

int
main (void)
{
  GC_INIT ();

  const char *df_root = gensym (EXPANDED (ABS_TOP_BUILDDIR)
                                "/tests/tmp.");

  size_t i0, i1, i2;
  void *data;
  size_t n_data;

  indexed_data_file_t df = create_indexed_data_file (df_root);

  write_to_indexed_data_file (df, "test data 0", 12, &i0);
  assert (i0 == 0);
  write_to_indexed_data_file (df, "tea time", 9, &i1);
  assert (i1 == 1);

  read_from_indexed_data_file (df, i1, &data, &n_data);
  assert (n_data == 9);
  assert (strcmp ((const char *) data, "tea time") == 0);

  read_from_indexed_data_file (df, i0, &data, &n_data);
  assert (n_data == 12);
  assert (strcmp ((const char *) data, "test data 0") == 0);

  close_indexed_data_file (df);
  reopen_indexed_data_file (df);

  write_to_indexed_data_file (df, "another entry", 14, &i2);

  read_from_indexed_data_file (df, i0, &data, &n_data);
  assert (n_data == 12);
  assert (strcmp ((const char *) data, "test data 0") == 0);

  read_from_indexed_data_file (df, i1, &data, &n_data);
  assert (n_data == 9);
  assert (strcmp ((const char *) data, "tea time") == 0);

  read_from_indexed_data_file (df, i2, &data, &n_data);
  assert (n_data == 14);
  assert (strcmp ((const char *) data, "another entry") == 0);

  close_indexed_data_file (df);

  df = open_indexed_data_file (df_root);

  read_from_indexed_data_file (df, i0, &data, &n_data);
  assert (n_data == 12);
  assert (strcmp ((const char *) data, "test data 0") == 0);

  read_from_indexed_data_file (df, i1, &data, &n_data);
  assert (n_data == 9);
  assert (strcmp ((const char *) data, "tea time") == 0);

  read_from_indexed_data_file (df, i2, &data, &n_data);
  assert (n_data == 14);
  assert (strcmp ((const char *) data, "another entry") == 0);

  remove_indexed_data_file (df);

  return 0;
}
