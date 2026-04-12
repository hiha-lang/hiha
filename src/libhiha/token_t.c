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
#include <string.h>
#include <error.h>
#include <exitfail.h>
#include <gl_avltree_list.h>
#include <gl_xlist.h>
#include <libhiha/token_t.h>

// Change this if using gettext.
#define _(msgid) msgid

#define VISIBLE [[gnu::visibility ("default")]]

struct token_getter_from_file
{
  /* This struct must be castable to a struct token_getter. */

  void (*get_token) (token_getter_t this_struct,
		     token_t *tok, const char **error_message);

  const char *filename;
  FILE *f;
  char *buf;
  size_t nbuf;
  gl_list_t lines;
  size_t line_no;		/* Starting at one. */
  size_t i_line;		/* Zero-based. */
  size_t i_code_point;		/* Zero-based. */
  bool eof_reached;
};

static void get_token_from_file (token_getter_t, token_t *tok,
				 const char **error_message);

VISIBLE token_getter_from_file_t
make_token_getter_from_file_t (const char *filename, FILE *f)
{
  token_getter_from_file_t getter =
    XMALLOC (struct token_getter_from_file);

  getter->get_token = &get_token_from_file;

  getter->filename = filename;
  getter->f = f;

  getter->nbuf = 1000;
  getter->buf = XNMALLOC (getter->nbuf, char);

  getter->lines =
    gl_list_create_empty (GL_AVLTREE_LIST, NULL, NULL, NULL,
			  false);
  getter->line_no = 0;
  getter->i_line = 0;
  getter->i_code_point = 0;

  getter->eof_reached = false;

  return getter;
}

static token_t
make_token_t (string_t token_kind, string_t token_value,
	      const char *filename, size_t line_no,
	      size_t code_point_no)
{
  token_t tok = XMALLOC (struct token);
  tok->token_kind = token_kind;
  tok->token_value = token_value;
  tok->loc = XMALLOC (struct text_location);
  tok->loc->filename = filename;
  tok->loc->line_no = line_no;
  tok->loc->code_point_no = code_point_no;
  return tok;
}

static bool
lines_need_refilling (token_getter_from_file_t g)
{
  return (!g->eof_reached && gl_list_size (g->lines) <= g->i_line);
}

static void
get_more_lines_if_necessary (token_getter_from_file_t g)
{
  text_location_t loc = XMALLOC (struct text_location);
  while (lines_need_refilling (g))
    {
      ssize_t nread = getline (&g->buf, &g->nbuf, g->f);
      g->eof_reached = (nread == -1);
      if (!g->eof_reached)
	{
	  loc->filename = g->filename;
	  loc->line_no = g->line_no + g->i_line;
	  loc->code_point_no = 0;
	  string_t str =
	    string_t_canonical_from_str_len (g->buf, nread, loc);
	  gl_list_add_last (g->lines, str);
	}
    }
}

static void
get_token_from_file (token_getter_t getter, token_t *tok,
		     const char **error_message)
{
  token_getter_from_file_t g = (token_getter_from_file_t) getter;

  *tok = NULL;
  *error_message = NULL;

  get_more_lines_if_necessary (g);
  if (g->eof_reached)
    *tok = make_token_t (make_string_t ("EOF"),
			 make_string_t ("EOF"),
			 g->filename, g->line_no + g->i_line,
			 g->i_code_point + 1);
  else
    {
      /// FIXME
    }
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
