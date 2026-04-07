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
#include <stdlib.h>
#include <progname.h>
#include <exitfail.h>
#include <xalloc.h>
#include <libhiha/string_t.h>
#include <libhiha/parse_expression.h>

char *line_buffer = NULL;
size_t line_buffer_size = 0;

static void
initialize_line_buffer (void)
{
  if (line_buffer == NULL)
    {
      line_buffer_size = 1000;
      line_buffer = XNMALLOC (line_buffer_size, char);
    }
}

static void
parse_file (const char *filename, FILE *f, parser_data_t parser_data)
{
  initialize_line_buffer ();
  size_t line_no = 0;
  ssize_t nread = getline (&line_buffer, &line_buffer_size, f);
  while (nread != -1)
    {
      line_no += 1;

      text_location_t loc = XMALLOC (struct text_location);
      loc->filename = make_string_t (filename);
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

void
check_usage (int argc, char **argv[[maybe_unused]])
{
  if (argc < 2)
    {
      printf ("Usage: %s src1.hiha src2.hiha ...\n", program_name);
      exit (exit_failure);
    }
}

int
main (int argc, char **argv)
{
  set_program_name (argv[0]);
  check_usage (argc, argv);
  parser_data_t parser_data = initialize_parser_data ();
  for (int i = 1; i != argc; i += 1)
    {
      FILE *f = fopen (argv[i], "r");
      if (f == NULL)
	{
	  printf ("%s: failed to open “%s” for reading\n",
		  program_name, argv[i]);
	  exit (exit_failure);
	}
      parse_file (argv[i], f, parser_data);
      fclose (f);
    }
  parser_data_t_free (parser_data);
  exit (EXIT_SUCCESS);
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
