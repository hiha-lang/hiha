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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include <locale.h>
#include <errno.h>
#include <dirent.h>
#include <filevercmp.h>
#include <progname.h>
#include <exitfail.h>
#include <error.h>
#include <xalloc.h>
#include <getopt.h>
#include <version-etc.h>
#include <gc/gc.h>
#include <libhiha/libhiha.h>

#define _(msgid) HIHA_GETTEXT (msgid)

#define NORETURN [[noreturn]]
#define MAYBE_UNUSED [[maybe_unused]]

#define GETOPT_HELP_CHAR (CHAR_MIN - 2)
#define GETOPT_VERSION_CHAR (CHAR_MIN - 3)
#define GETOPT_PLUGIN_CHAR (CHAR_MAX + 1)
#define GETOPT_PLUGINDIR_CHAR (CHAR_MAX + 2)
#define GETOPT_DUMP_TOKENS_CHAR (CHAR_MAX + 3)

enum hiha_plugin_tag_t
{
  TAG_PLUGIN,
  TAG_PLUGINDIR
};
typedef enum hiha_plugin_tag_t hiha_plugin_tag_t;

struct hiha_plugin
{
  hiha_plugin_tag_t tag;
  const char *locator;
};
typedef struct hiha_plugin *hiha_plugin_t;

struct hiha_options
{
  voidp_vector_t plugins;
  const char *dump_tokens_filename;
};
typedef struct hiha_options *hiha_options_t;

HIHA_VISIBLE const char version_etc_copyright[] =
  "Copyright %s %d Barry Schwartz";

#define COMMAND_NAME "hiha"

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
  usage_puts (_("Do something not yet implemented "
                "with hiha source FILES...\n"));
  usage_puts ("\n");
  usage_puts (_("  --plugin=PLUGIN      load a plugin\n"));
  usage_puts (_("  --plugindir=DIR      "
                "load plugins from a directory\n"));
  usage_puts (_("  --dump-tokens=FILE   dump tokens "
                "(“-” for standard output)\n"));
  usage_puts (_("  --help               "
                "display this help and exit\n"));
  usage_puts (_("  --version            "
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
  {"plugindir", required_argument, NULL, GETOPT_PLUGINDIR_CHAR},
  {"dump-tokens", required_argument, NULL, GETOPT_DUMP_TOKENS_CHAR},
  {"help", no_argument, NULL, GETOPT_HELP_CHAR},
  {"version", no_argument, NULL, GETOPT_VERSION_CHAR},
  {NULL, 0, NULL, 0}
};

static int
getopt_for_this_program (int argc, char **argv)
{
  return getopt_long (argc, argv, "", long_opts, NULL);
}

static hiha_plugin_t
make_hiha_plugin_t (hiha_plugin_tag_t tag, const char *locator)
{
  hiha_plugin_t p = XMALLOC (struct hiha_plugin);
  p->tag = tag;
  p->locator = xstrdup (locator);
  return p;
};

static void
require_a_digit_and_only_digits (const char *option, const char *s)
{
  size_t i = 0;
  while ('0' <= s[i] && s[i] <= '9')
    i += 1;
  if (i == 0 || s[i] != '\0')
    {
      error (exit_failure, 0, _("%s: expected digits but got “%s”"),
             option, s);
      abort ();
    }
}

static void
get_options (int argc, char **argv, hiha_options_t *opts)
{
  *opts = XMALLOC (struct hiha_options);
  (*opts)->plugins = NULL;
  (*opts)->dump_tokens_filename = NULL;

  int c = getopt_for_this_program (argc, argv);
  while (c != -1)
    {
      switch (c)
        {
        case GETOPT_PLUGIN_CHAR:
          (*opts)->plugins =
            voidp_vector_push ((*opts)->plugins,
                               make_hiha_plugin_t (TAG_PLUGIN, optarg));
          break;

        case GETOPT_PLUGINDIR_CHAR:
          (*opts)->plugins =
            voidp_vector_push ((*opts)->plugins,
                               make_hiha_plugin_t (TAG_PLUGINDIR,
                                                   optarg));
          break;

        case GETOPT_DUMP_TOKENS_CHAR:
          (*opts)->dump_tokens_filename = xstrdup (optarg);
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

void
load_one_plugin (const char *fn)
{
  const char *error_message = NULL;
  load_plugin (fn, &error_message);
  if (error_message != NULL)
    {
      error (exit_failure, 0, "%s", error_message);
      abort ();
    }
}

int
plugin_filter (const struct dirent *dp)
{
  bool accept = false;
  if (dp->d_name[0] != '.' && strncmp (dp->d_name, "lib", 3) != 0)
    {
      const char *p = strstr (dp->d_name, ".so");
      if (p != NULL)
        accept = (p - dp->d_name == strlen (dp->d_name) - 3);
    }
  return accept;
}

static int
_fileversort (const struct dirent **a, const struct dirent **b)
{
  return filevercmp ((*a)->d_name, (*b)->d_name);
}

void
load_one_plugindir (const char *dirname)
{
  struct dirent **namelist;
  int num_entries =
    scandir (dirname, &namelist, plugin_filter, _fileversort);
  if (num_entries < 0)
    {
      int err_number = errno;
      char *msg = strerror (err_number);
      error (exit_failure, err_number, "%s", msg);
      abort ();
    }
  else
    {
      const int n_dirname = strlen (dirname);
      for (int i = 0; i != num_entries; i += 1)
        {
          char *dn = namelist[i]->d_name;
          size_t n_dn = strlen (dn);
          char *fn = XCALLOC (n_dn + n_dirname + 2, char);
          memcpy (fn, dirname, n_dirname * sizeof (char));
          fn[n_dirname] = '/';
          memcpy (fn + n_dirname + 1, namelist[i]->d_name, n_dn);
          load_one_plugin (fn);
        }
    }
}

static void
load_command_line_plugins (voidp_vector_t plugins)
{
  for (size_t i = 0; i != voidp_vector_length (plugins); i += 1)
    {
      const hiha_plugin_t p =
        (hiha_plugin_t) voidp_vector_ref (plugins, i);
      switch (p->tag)
        {
        case TAG_PLUGIN:
          load_one_plugin (p->locator);
          break;
        case TAG_PLUGINDIR:
          load_one_plugindir (p->locator);
          break;
        }
    }
}

static void
open_dump_tokens_stream (const char *filename)
{
  if (filename == NULL)
    dump_tokens_stream = NULL;
  else if (strcmp (filename, "-") == 0)
    dump_tokens_stream = stdout;
  else
    {
      dump_tokens_stream = fopen (filename, "w");
      int err_number = errno;
      if (dump_tokens_stream == NULL)
        {
          error (exit_failure, err_number, "%s", filename);
          abort ();
        }
    }
}

static void
close_dump_tokens_stream (void)
{
  if (dump_tokens_stream != NULL)
    if (dump_tokens_stream != stdout)
      if (dump_tokens_stream != stderr)
        fclose (dump_tokens_stream);
}

static bool
pass_predicate (const char *pass)
{
  return true;
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

  load_command_line_plugins (opts->plugins);
  open_dump_tokens_stream (opts->dump_tokens_filename);

  const char *final_filename_root;
  const char *error_message;
  parsing_state_t parsing_state = make_parsing_state_t ();
  do_pratt_passes (parsing_state, pass_predicate,
                   argc - 1, ((const char **) argv) + 1,
                   "pass", &final_filename_root, &error_message);

  close_dump_tokens_stream ();

  if (error_message != NULL)
    {
      error (exit_failure, 0, "%s", error_message);
      exit (EXIT_FAILURE);
    }

  exit (EXIT_SUCCESS);
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
