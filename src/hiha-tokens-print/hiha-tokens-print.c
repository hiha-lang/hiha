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
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <locale.h>
#include <progname.h>
#include <exitfail.h>
#include <error.h>
#include <xalloc.h>
#include <getopt.h>
#include <version-etc.h>
#include <gc/gc.h>
#include <gl_avltree_list.h>
#include <gl_xlist.h>
#include <libhiha/libhiha.h>

// Change this if using gettext.
#define _(msgid) msgid

#define NORETURN [[noreturn]]
#define MAYBE_UNUSED [[maybe_unused]]

#define GETOPT_HELP_CHAR (CHAR_MIN - 2)
#define GETOPT_VERSION_CHAR (CHAR_MIN - 3)

struct hiha_options
{
  gl_list_t plugins;
};
typedef struct hiha_options *hiha_options_t;

HIHA_VISIBLE const char version_etc_copyright[] =
  "Copyright %s %d Barry Schwartz";

#define COMMAND_NAME "hiha-tokens-print"

static const char *authors[] = {
  "Barry Schwartz",
  NULL
};

static void
print_version (void)
{
  version_etc_ar (stdout, COMMAND_NAME,
                  _("hiha: an “orthogonal” programming language"),
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
  usage_puts (_("Read files of hiha tokens and print "
                "the tokens nicely, in\nthe current locale,\n"
                "to standard output.\n"));
  usage_puts ("\n");
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
        case GETOPT_VERSION_CHAR:
          print_version_then_exit ();

        case GETOPT_HELP_CHAR:
        default:
          print_usage_then_exit ();
        }
      c = getopt_for_this_program (argc, argv);
    }
}

static void
exit_if_error (const char *error_message)
{
  if (error_message != NULL)
    {
      error (exit_failure, 0, "%s", error_message);
      abort ();
    }
}

static void
print_tokens (const char *filename)
{
  FILE *f = fopen (filename, "r");
  int err_number = errno;
  if (f == NULL)
    {
      error (exit_failure, err_number, "%s", filename);
      abort ();
    }
  token_getter_t getter =
    make_token_getter_from_serialized_tokens_t (filename, f);

  const char *error_message;
  token_t tok;

  getter->get_token (getter, &tok, &error_message);
  exit_if_error (error_message);
  while (0 != string_t_cmp (tok->token_kind, string_t_EOF ()))
    {
      print_token_t (tok, stdout);
      getter->get_token (getter, &tok, &error_message);
      exit_if_error (error_message);
    }
  print_token_t (tok, stdout);
}

int
main (int argc, char **argv)
{
  GC_INIT ();
  setlocale (LC_ALL, "");
  set_program_name (argv[0]);

  hiha_options_t opts;
  get_options (argc, argv, &opts);
  argc -= optind - 1;
  argv += optind - 1;

  check_usage (argc, argv);

  if (2 <= argc)
    {
      for (size_t i = 1; i != argc; i += 1)
        print_tokens (argv[i]);
    }

  exit (EXIT_SUCCESS);
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
