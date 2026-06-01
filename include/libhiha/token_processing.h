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

#ifndef __LIBHAHA__TOKEN_PROCESSING_H__INCLUDED__
#define __LIBHAHA__TOKEN_PROCESSING_H__INCLUDED__

#include <libhiha/token_t.h>
#include <libhiha/pratt.h>

/* Scan source files to a set of files
   /tmp/hiha.<XXXXX>/<filename_root>.{tokens,data,index} */
void scan_source_files (size_t n, const char *filenames[n],
                        const char *filename_root,
                        const char **error_message);

void do_pratt_pass (void *state, buffered_token_getter_t getter,
                    token_putter_t putter,
                    pratt_tables_t tables, const char **error_message);

void do_pratt_pass_for_files (void *state,
                              const char *source_filename_root,
                              const char *destination_filename_root,
                              pratt_tables_t tables,
                              bool remove_source_files,
                              const char **error_message);

void do_pratt_passes (void *state, bool (*pred) (const char *),
                      size_t n, const char *filenames[n],
                      const char *filename_root,
                      const char **final_filename_root,
                      const char **error_message);

#endif /* __LIBHAHA__TOKEN_PROCESSING_H__INCLUDED__ */

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
