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
#include <pcre2.h>
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
  void (*free) (token_getter_t this_struct);

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

VISIBLE void
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

static void
free_string (const void *s)
{
  string_t str = (string_t) s;
  string_t_free (str);
}

static void get_token_from_file (token_getter_t, token_t *tok,
				 const char **error_message);
static void token_getter_from_file_t_free (token_getter_t);

VISIBLE token_getter_from_file_t
make_token_getter_from_file_t (const char *filename, FILE *f)
{
  token_getter_from_file_t getter =
    XMALLOC (struct token_getter_from_file);

  getter->get_token = &get_token_from_file;
  getter->free = &token_getter_from_file_t_free;

  getter->filename = filename;
  getter->f = f;

  getter->nbuf = 1000;
  getter->buf = XNMALLOC (getter->nbuf, char);

  getter->lines =
    gl_list_create_empty (GL_AVLTREE_LIST, NULL, NULL, free_string,
			  false);
  getter->line_no = 0;
  getter->i_line = 0;
  getter->i_code_point = 0;

  getter->eof_reached = false;

  return getter;
}

static void
token_getter_from_file_t_free (token_getter_t getter)
{
  if (getter != NULL)
    {
      token_getter_from_file_t g = (token_getter_from_file_t) getter;
      free (g->buf);
      gl_list_free (g->lines);
      free (g);
    }
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
  text_location_t_free (loc);
}

static void
match_pattern (string_t pattern, const pcre2_code **re,
	       const string_t subject, size_t i_code_point,
	       string_t *match)
{
  if (*re == NULL)
    {
      int errorcode;
      PCRE2_SIZE erroroffset;
      *re = pcre2_compile (pattern->s, pattern->n,
			   PCRE2_ANCHORED | PCRE2_UTF | PCRE2_UCP,
			   &errorcode, &erroroffset, NULL);
      if (*re == NULL)
	error (exit_failure, 0, "%s",
	       _("regular expression compilation failed"));
    }
  pcre2_match_data *match_data =
    pcre2_match_data_create_from_pattern (*re, NULL);
  int retval = pcre2_match (*re, subject->s, subject->n, i_code_point,
			    PCRE2_ANCHORED, match_data, NULL);
  if (retval <= 0)
    *match = NULL;
  else
    {
      PCRE2_SIZE *ovector = pcre2_get_ovector_pointer (match_data);
      *match = XMALLOC (struct string);
      (*match)->n = ovector[1] - ovector[0];
      (*match)->s = XNMALLOC ((*match)->n, uint32_t);
      memcpy ((*match)->s, subject->s + ovector[0],
	      (*match)->n * sizeof (uint32_t));
    }
  pcre2_match_data_free (match_data);
}

static void
match_whitespace (const string_t subject, const char *filename,
		  size_t line_no, size_t i_code_point, token_t *tok)
{
  string_t pattern = make_string_t ("\\w+");
  static const pcre2_code *re = NULL;

  string_t match;
  match_pattern (pattern, &re, subject, i_code_point, &match);
  if (match == NULL)
    *tok = NULL;
  else
    *tok = make_token_t (make_string_t ("WHITESPACE"), match,
			 filename, line_no, i_code_point + 1);
  string_t_free (pattern);
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
      // FIXME: FOR NOW WE DO THIS, BUT IN FACT WE WILL HAVE PLUGINS
      // AND SUCH.
      string_t pattern = make_string_t ("");
      int errorcode;
      PCRE2_SIZE erroroffset;
      pcre2_code *re = pcre2_compile (pattern->s, pattern->n,
				      PCRE2_ANCHORED | PCRE2_UTF |
				      PCRE2_UCP,
				      &errorcode, &erroroffset, NULL);
      if (re == NULL)
	error (exit_failure, 0, "%s",
	       _("regular expression compilation failed"));
      //      int pcre2_match(const pcre2_code *code, PCRE2_SPTR subject, PCRE2_SIZE length, g->i_code_point, PCRE2_ANCHORED, pcre2_match_data *match_data, NULL);
      pcre2_code_free (re);
      string_t_free (pattern);
    }
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
