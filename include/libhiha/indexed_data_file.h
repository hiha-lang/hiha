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

#ifndef __LIBHAHA__INDEXED_DATA_FILE_H__INCLUDED__
#define __LIBHAHA__INDEXED_DATA_FILE_H__INCLUDED__

struct indexed_data_file;
typedef struct indexed_data_file *indexed_data_file_t;

indexed_data_file_t create_indexed_data_file (const char
                                              *filename_root);
indexed_data_file_t open_indexed_data_file (const char *filename_root);
void close_indexed_data_file (indexed_data_file_t df);
void reopen_indexed_data_file (indexed_data_file_t df);

void write_to_indexed_data_file (indexed_data_file_t df,
                                 const void *data, size_t n_data,
                                 size_t *index);
void read_from_indexed_data_file (indexed_data_file_t df, size_t index,
                                  void **data, size_t *n_data);

/* The following will close the files if they are open. */
void remove_indexed_data_file (indexed_data_file_t df);

#endif /* __LIBHAHA__INDEXED_DATA_FILE_H__INCLUDED__ */

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
