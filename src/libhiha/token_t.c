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
#include <xalloc.h>
#include <exitfail.h>
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
  size_t nbuf;			/* The size of the buf. */
  size_t n;			/* How many are in the buf. */
  size_t line_no;		/* Starting at one. */
  size_t i_code_point;		/* Zero-based. */
  bool eof_reached;
};

static void get_token_from_file (token_getter_t, token_t *tok,
				 const char **error_message);

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

  getter->n = 0;
  getter->line_no = 0;
  getter->i_code_point = 0;

  getter->eof_reached = false;

  return getter;
}

static void
fill_line_if_necessary (token_getter_from_file_t g)
{
  if (!g->eof_reached && g->i_code_point == g->n)
    {
      ssize_t nread = getline (&g->buf, &g->nbuf, g->f);
      g->eof_reached = (nread == -1);
      if (!g->eof_reached)
	{
	  g->n = nread;
	  g->line_no += 1;
	  g->i_code_point = 0;
	}
    }
}

static void
get_token_from_file (token_getter_t getter, token_t *tok,
		     const char **error_message)
{
  token_getter_from_file_t g = (token_getter_from_file_t) getter;
  *error_message = NULL;
  fill_line_if_necessary (g);
  if (g->eof_reached)
    *tok =
      make_token_t (copy_string_t (string_t_EOF),
		    copy_string_t (string_t_EOF), g->filename,
		    g->line_no, g->i_code_point + 1);
  else
    {
      string_t str = XMALLOC (struct string);
      str->s = XNMALLOC (1, uint32_t);
      str->s[0] = g->buf[g->i_code_point];
      str->n = 1;
      *tok = make_token_t (copy_string_t (string_t_CODE_POINT), str,
			   g->filename, g->line_no,
			   g->i_code_point + 1);
      g->i_code_point += 1;
    }
}

VISIBLE void
print_token_t (const token_t tok, FILE *f)
{
  const char *separator = "  ";
  print_string_t (tok->token_kind, f);
  fputs (separator, f);
  print_string_t (tok->token_value, f);
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
