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
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <exitfail.h>
#include <xalloc.h>
#include <libhiha/indexed_data_file.h>

#define _(msgid) HIHA_GETTEXT (msgid)

#define CHECK_FILE_ERROR(pred, filename)                        \
  do                                                            \
    {                                                           \
      const int _errnum__ = errno;                              \
      if (pred)                                                 \
        {                                                       \
          error (exit_failure, _errnum__, "%s", (filename));    \
          abort ();                                             \
        }                                                       \
    }                                                           \
  while (0)

struct indexed_data_file
{
  const char *filename_data;
  const char *filename_index;
  FILE *f_data;
  FILE *f_index;
};

static void
require_open (indexed_data_file_t df)
{
  assert ((df->f_data == NULL) == (df->f_index == NULL));

  if (df->f_data == NULL)
    {
      error (exit_failure, 0, "file is not open: %s",
             df->filename_data);
      abort ();
    }
}

static size_t
next_index (indexed_data_file_t df)
{
  require_open (df);

  int retval = fseek (df->f_index, 0, SEEK_END);
  CHECK_FILE_ERROR (retval != 0, df->filename_index);
  long pos = ftell (df->f_index);
  CHECK_FILE_ERROR (pos == -1, df->filename_index);

  return ((size_t) ((intmax_t) pos) / (intmax_t) sizeof (intmax_t));
}

static intmax_t
next_offset (indexed_data_file_t df)
{
  /* The current size of the data file is the offset to the next
     entry, once it gets written. */

  require_open (df);

  int retval = fseek (df->f_data, 0, SEEK_END);
  CHECK_FILE_ERROR (retval != 0, df->filename_data);
  long pos = ftell (df->f_data);
  CHECK_FILE_ERROR (pos == -1, df->filename_data);

  return (intmax_t) pos;
}

static void
_open_indexed_data_file (indexed_data_file_t df, const char *mode)
{
  /* Open the file. It must not be open already. */

  assert ((df->f_data == NULL) == (df->f_index == NULL));
  assert (df->f_data == NULL);

  df->f_data = fopen (df->filename_data, mode);
  CHECK_FILE_ERROR (df->f_data == NULL, df->filename_data);

  df->f_index = fopen (df->filename_index, mode);
  CHECK_FILE_ERROR (df->f_index == NULL, df->filename_index);
}

static indexed_data_file_t
_allocate_indexed_data_file (const char *filename_root,
                             const char *mode)
{
  size_t n_root = strlen (filename_root);
  char *filename_data = XCALLOC (n_root + 6, char);
  char *filename_index = XCALLOC (n_root + 7, char);
  memcpy (filename_data, filename_root, n_root * sizeof (char));
  memcpy (filename_data + n_root, ".data", 5 * sizeof (char));
  memcpy (filename_index, filename_root, n_root * sizeof (char));
  memcpy (filename_index + n_root, ".index", 6 * sizeof (char));

  indexed_data_file_t df = XMALLOC (struct indexed_data_file);
  df->filename_data = filename_data;
  df->filename_index = filename_index;

  df->f_data = NULL;
  df->f_index = NULL;
  _open_indexed_data_file (df, mode);

  return df;
}

HIHA_VISIBLE indexed_data_file_t
create_indexed_data_file (const char *filename_root)
{
  return _allocate_indexed_data_file (filename_root, "wb+");
}

HIHA_VISIBLE indexed_data_file_t
open_indexed_data_file (const char *filename_root)
{
  return _allocate_indexed_data_file (filename_root, "rb+");
}

HIHA_VISIBLE void
close_indexed_data_file (indexed_data_file_t df)
{
  /* Close the file, unless it already is closed. */

  assert ((df->f_data == NULL) == (df->f_index == NULL));

  if (df->f_data != NULL)
    {
      fclose (df->f_data);
      fclose (df->f_index);

      df->f_data = NULL;
      df->f_index = NULL;
    }
}

HIHA_VISIBLE void
reopen_indexed_data_file (indexed_data_file_t df)
{
  /* Reopen the file, unless it already is open. */

  assert ((df->f_data == NULL) == (df->f_index == NULL));

  if (df->f_data == NULL)
    _open_indexed_data_file (df, "rb+");
}

HIHA_VISIBLE void
write_to_indexed_data_file (indexed_data_file_t df, const void *data,
                            size_t n_data, size_t *index)
{
  require_open (df);

  int retval;
  size_t num_written;

  intmax_t offset = next_offset (df);

  retval = fseek (df->f_data, 0, SEEK_END);
  CHECK_FILE_ERROR (retval != 0, df->filename_data);
  num_written = fwrite (&n_data, sizeof (size_t), 1, df->f_data);
  CHECK_FILE_ERROR (num_written != 1, df->filename_data);
  num_written = fwrite (data, 1, n_data, df->f_data);
  CHECK_FILE_ERROR (num_written != n_data, df->filename_data);

  *index = next_index (df);

  retval = fseek (df->f_index, 0, SEEK_END);
  CHECK_FILE_ERROR (retval != 0, df->filename_index);
  num_written = fwrite (&offset, sizeof (intmax_t), 1, df->f_index);
  CHECK_FILE_ERROR (num_written != 1, df->filename_index);
}

HIHA_VISIBLE void
read_from_indexed_data_file (indexed_data_file_t df, size_t index,
                             void **data, size_t *n_data)
{
  require_open (df);

  int retval;
  int num_read;
  intmax_t offset;

  retval = fseek (df->f_index, index * sizeof (intmax_t), SEEK_SET);
  CHECK_FILE_ERROR (retval != 0, df->filename_index);
  num_read = fread (&offset, sizeof (intmax_t), 1, df->f_index);
  CHECK_FILE_ERROR (num_read != 1, df->filename_index);

  retval = fseek (df->f_data, offset, SEEK_SET);
  CHECK_FILE_ERROR (retval != 0, df->filename_data);
  num_read = fread (n_data, sizeof (size_t), 1, df->f_data);
  CHECK_FILE_ERROR (num_read != 1, df->filename_data);
  *data = xmalloc (*n_data);
  num_read = fread (*data, 1, *n_data, df->f_data);
  CHECK_FILE_ERROR (num_read != *n_data, df->filename_data);
}

HIHA_VISIBLE void
remove_indexed_data_file (indexed_data_file_t df)
{
  assert ((df->f_data == NULL) == (df->f_index == NULL));

  if (df->f_data != NULL)
    close_indexed_data_file (df);

  int retval;

  retval = remove (df->filename_data);
  CHECK_FILE_ERROR (retval != 0, df->filename_data);
  retval = remove (df->filename_index);
  CHECK_FILE_ERROR (retval != 0, df->filename_index);

  df->filename_data = NULL;
  df->filename_index = NULL;
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
