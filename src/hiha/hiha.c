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
#include <limits.h>
#include <progname.h>
#include <exitfail.h>
#include <xalloc.h>
#include <getopt.h>
#include <version-etc.h>
#include <gc/gc.h>
#include <gl_avltree_list.h>
#include <gl_xlist.h>
#include <libhiha/string_t.h>
#include <libhiha/token_t.h>
#include <libhiha/parse_expression.h>

// Change this if using gettext.
#define _(msgid) msgid

#define VISIBLE [[gnu::visibility ("default")]]
#define NORETURN [[noreturn]]
#define MAYBE_UNUSED [[maybe_unused]]

#define GETOPT_HELP_CHAR (CHAR_MIN - 2)
#define GETOPT_VERSION_CHAR (CHAR_MIN - 3)
#define GETOPT_PLUGIN_CHAR (CHAR_MAX + 1)

struct hiha_options
{
  gl_list_t plugins;
};
typedef struct hiha_options *hiha_options_t;

VISIBLE const char version_etc_copyright[] =
  "Copyright %s %d Barry Schwartz";

#define COMMAND_NAME "hiha"

static const char *authors[] = {
  "Barry Schwartz",
  NULL
};

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
  token_t tok;
  const char *error_message;

  token_getter_from_source_file_t g =
    make_token_getter_from_source_file_t (filename, f);
  token_getter_t getter = (token_getter_t) g;

  (getter->get_token) (getter, &tok, &error_message);
  while (!error_message
	 && string_t_cmp (tok->token_kind, string_t_EOF ()))
    {
      serialize_token_t (tok, stdout);
      //print_token_t (tok, stdout);
      //fputs ("\n", stdout);
      (getter->get_token) (getter, &tok, &error_message);
    }
  if (!error_message)
    {
      serialize_token_t (tok, stdout);
      //print_token_t (tok, stdout);
      //fputs ("\n", stdout);
    }
}

static void
print_version (void)
{
  version_etc_ar (stdout, COMMAND_NAME,
		  "an “orthogonal” programming language",
		  PACKAGE_VERSION, authors);
}

NORETURN static void
print_version_then_exit (void)
{
  print_version ();
  exit (exit_failure);
}

static void
usage_puts (const char *s)
{
  fputs (s, stdout);
}

static void
print_usage (void)
{
  usage_puts (_("Usage: "));
  usage_puts (program_name);
  usage_puts (_(" [OPTION] FILES...\n"));
  usage_puts (_("Do something not yet implemented "
		"with hiha source FILES...\n"));
  usage_puts ("\n");
  usage_puts (_("      --plugin=PLUGIN     load the plugin\n"));
  usage_puts (_("      --help        display this help and exit\n"));
  usage_puts (_("      --version     "
		"output version information and exit\n"));
  usage_puts ("\n");
  usage_puts (_("hiha homepage: "
		"<https://github.com/chemoelectric/hiha>\n"));
}

NORETURN static void
print_usage_then_exit (void)
{
  print_usage ();
  exit (exit_failure);
}

static void
check_usage (int argc, MAYBE_UNUSED char **argv)
{
  if (argc < 2)
    print_usage_then_exit ();
}

static struct option const long_opts[] = {
  {"plugin", required_argument, NULL, GETOPT_PLUGIN_CHAR},
  {"help", no_argument, NULL, GETOPT_HELP_CHAR},
  {"version", no_argument, NULL, GETOPT_VERSION_CHAR},
  {NULL, 0, NULL, 0}
};

static int
getopt_for_this_program (int argc, char **argv)
{
  return getopt_long (argc, argv, "", long_opts, NULL);
}

static void
get_options (int argc, char **argv, hiha_options_t *opts)
{
  *opts = XMALLOC (struct hiha_options);
  (*opts)->plugins = gl_list_create_empty (GL_AVLTREE_LIST, NULL, NULL,
					   NULL, true);

  int c = getopt_for_this_program (argc, argv);
  while (c != -1)
    {
      switch (c)
	{
	case GETOPT_PLUGIN_CHAR:
	  gl_list_add_last ((*opts)->plugins, xstrdup (optarg));
	  break;

	case GETOPT_VERSION_CHAR:
	  print_version_then_exit ();

	case GETOPT_HELP_CHAR:
	default:
	  print_usage_then_exit ();
	}
      c = getopt_for_this_program (argc, argv);
    }
}

int
main (int argc, char **argv)
{
  GC_INIT ();
  set_program_name (argv[0]);

  hiha_options_t opts;
  get_options (argc, argv, &opts);
  argc -= optind - 1;
  argv += optind - 1;

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
  exit (EXIT_SUCCESS);
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
