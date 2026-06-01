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
#include <math.h>
#include <libhiha/token_processing.h>


static void
scan_tokens (buffered_token_getter_t getter, token_putter_t putter,
             const char **error_message)
{
  token_t tok;
  getter->get_token (getter, &tok, error_message);
  while (*error_message == NULL && !token_t_is_eof_eof (tok))
    {
      putter->put_token (putter, tok, error_message);
      if (*error_message == NULL)
        getter->get_token (getter, &tok, error_message);
    }
  if (*error_message == NULL)
    putter->put_token (putter, tok, error_message);
}

HIHA_VISIBLE void
scan_source_files (size_t n, const char *filenames[n],
                   const char *filename_root,
                   const char **error_message)
{
  *error_message = NULL;

  buffered_token_getter_t getter =
    make_buffered_token_getter_from_source_files (n, filenames);
  token_putter_to_files_t putter =
    make_token_putter_to_files_t (filename_root);

  scan_tokens (getter, to_token_putter_t (putter), error_message);

  if (*error_message == NULL)
    close_token_putter_files (putter);
}

HIHA_VISIBLE void
do_pratt_pass (void *state, buffered_token_getter_t getter,
               token_putter_t putter, pratt_tables_t tables,
               const char **error_message)
{
  token_t tok;

  *error_message = NULL;

  pratt_parse (state, getter, tables, -HUGE_VAL, &tok, error_message);
  while (*error_message == NULL && !token_t_is_eof_eof (tok))
    {
      putter->put_token (putter, tok, error_message);
      if (*error_message == NULL)
        pratt_parse (state, getter, tables, -HUGE_VAL, &tok,
                     error_message);
    }
  if (*error_message == NULL)
    putter->put_token (putter, tok, error_message);
}

HIHA_VISIBLE void
do_pratt_pass_for_files (void *state, const char *source_filename_root,
                         const char *destination_filename_root,
                         pratt_tables_t tables,
                         bool remove_source_files,
                         const char **error_message)
{
  *error_message = NULL;

  token_getter_from_files_t getter =
    make_token_getter_from_files_t (source_filename_root);
  token_putter_to_files_t putter =
    make_token_putter_to_files_t (destination_filename_root);

  buffered_token_getter_t tok_getter =
    make_buffered_token_getter_t (to_token_getter_t (getter));
  token_putter_t tok_putter = to_token_putter_t (putter);

  do_pratt_pass (state, tok_getter, tok_putter, tables, error_message);

  if (*error_message == NULL)
    {
      close_token_getter_files (getter);
      close_token_putter_files (putter);
      if (remove_source_files)
        remove_token_getter_files (getter);
    }
}

static const char *
extend_filename_root (const char *filename_root, size_t i)
{
  size_t n_fn = strlen (filename_root);
  size_t n = n_fn + 17;
  char *s = XNMALLOC (n, char);
  snprintf (s, n, "%-*s%016zx", n_fn, filename_root, i);
  return s;
}

HIHA_VISIBLE void
do_pratt_passes (void *state, bool (*pred) (const char *),
                 size_t n, const char *filenames[n],
                 const char *filename_root,
                 const char **final_filename_root,
                 const char **error_message)
{
  const char *fn_root1;
  const char *fn_root2;

  *error_message = NULL;

  pratt_tables_vector_t vec = get_pratt_tables ();
  size_t n_vec = pratt_tables_vector_length (vec);

  size_t i = 0;
  fn_root1 = extend_filename_root (filename_root, i);
  scan_source_files (n, filenames, fn_root1, error_message);
  while (*error_message == NULL && i != n_vec)
    {
      pratt_tables_entry_t entry = pratt_tables_vector_ref (vec, i);
      if (pred (entry.pass))
        {
          fn_root2 = extend_filename_root (filename_root, i + 1);
          do_pratt_pass_for_files (state, fn_root1, fn_root2,
                                   entry.tables, true, error_message);
          fn_root1 = fn_root2;
        }
      i += 1;
    }
  *final_filename_root = fn_root1;
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
