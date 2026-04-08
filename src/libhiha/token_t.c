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
#include <stdio.h>
#include <gl_avltree_list.h>
#include <gl_xlist.h>
#include <libhiha/token_t.h>

// Change this if using gettext.
#define _(msgid) msgid

struct token_getter_from_file
{
  /* This struct must be castable to a struct token_getter. */

  void (*get_token) (token_getter_t this_struct,
                     token_t *tok, const char **error_message);
  void (*free) (token_getter_t this_struct);

  const char *filename;
  FILE *f;
  gl_list_t lines_buffer;
  size_t line_no;               /* Starting at one. */
  size_t i_line;                /* Zero-based. */
  size_t i_code_point;          /* Zero-based. */
  bool eof_reached;
};

void
token_t_free (token_t tok)
{
  if (tok != NULL)
    {
      string_t_free (tok->token_kind);
      string_t_free (tok->token_value);
      text_location_t_free (tok->loc);
    }
  free (tok);
}

/*
  static void
get_token_from_file (const char *filename, FILE *f, parser_data_t parser_data)
{
  initialize_line_buffer ();
  size_t line_no = 0;
  ssize_t nread = getline (&line_buffer, &line_buffer_size, f);
  while (nread != -1)
    {
      line_no += 1;

      text_location_t loc = XMALLOC (struct text_location);
      loc->filename = filename;
      loc->line_no = line_no;
      loc->code_point_no = 0;
      string_t str =
	string_t_canonical_from_str_len (line_buffer, nread, loc);

      char *s;
      size_t n;
      str_len_from_string_t (str, &s, &n);
      printf ("%06zu ", line_no);
      fwrite (s, n, sizeof (char), stdout);
      free (s);

      text_location_t_free (loc);
      string_t_free (str);
      nread = getline (&line_buffer, &line_buffer_size, f);
    }
}
*/

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
