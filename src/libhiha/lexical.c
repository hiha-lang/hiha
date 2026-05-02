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

/*

  Lexical analysis via Pratt parser
  ---------------------------------

  Use the Pratt parser to convert token streams to token streams.
  Continue until a fixed point is reached.

*/

#include <config.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <error.h>
#include <errno.h>
#include <exitfail.h>
#include <xalloc.h>
#include <xstrerror.h>
#include <libhiha/lexical.h>
#include <libhiha/initialize_once.h>
#include <libhiha/workspaces.h>

// Change this if using gettext.
#define _(msgid) msgid

HIHA_VISIBLE size_t lexical_max = 100;

static initialize_once_t _lexical_tables_init1t =
  INITIALIZE_ONCE_T_INIT;
static pratt_tables_t _lexical_tables = NULL;

static void
_initialize_lexical_tables (void)
{
  _lexical_tables = make_pratt_tables_t ();
}

HIHA_VISIBLE pratt_tables_t
lexical_pratt_tables (void)
{
  INITIALIZE_ONCE (_lexical_tables_init1t, _initialize_lexical_tables);
  return _lexical_tables;
}

static token_t
lhs_to_token_t (void *lhs, const char *error_message)
{
  return (error_message == NULL) ? (token_t) lhs : NULL;
}

HIHA_VISIBLE void
scan_tokens (void *state, buffered_token_getter_t getter,
             token_putter_t putter, const char **error_message)
{
  token_t tok;
  void *lhs = NULL;
  pratt_tables_t tables = lexical_pratt_tables ();

  *error_message = NULL;

  pratt_parse (state, getter, tables, -HUGE_VAL, &lhs, error_message);
  tok = lhs_to_token_t (lhs, *error_message);
  while (*error_message == NULL && !token_t_is_eof_eof (tok))
    {
      putter->put_token (putter, tok, error_message);
      if (*error_message == NULL)
        {
          pratt_parse (state, getter, tables, -HUGE_VAL, &lhs,
                       error_message);
          tok = lhs_to_token_t (lhs, *error_message);
        }
    }
  if (*error_message == NULL)
    putter->put_token (putter, tok, error_message);
}

static void
open_file (const char *filename, const char *mode, FILE **f,
           const char **error_message)
{
  *error_message = NULL;
  *f = fopen (filename, mode);
  int err_number = errno;
  if (f == NULL)
    {
      const char *msg = xstrerror (NULL, err_number);
      char s[1000];
      snprintf (s, 1000, "error: %s: %s", msg, filename);
      *error_message = xstrdup (s);
    }
}

/* Scan back and forth between filename[0] and filename[1], until a
   fixed point is reached. The initial and final files will both be
   filename[*ifile]. */
HIHA_VISIBLE void
scan_serialized_tokens_until_fixed_point (const char *filename[2],
                                          size_t *ifile,
                                          const char **error_message)
{
  assert (*ifile <= 1);
  *ifile = 1 - *ifile;

  *error_message = NULL;

  /* One pass for having read the source files. */
  size_t ipass = 1;

  bool done = false;
  while (!done)
    if (ipass == lexical_max)
      {
        char s[1000];
        snprintf (s, 1000,
                  _("lexical fixed point not reached within %zu %s"),
                  lexical_max,
                  (lexical_max == 1) ? _("pass") : _("passes"));
        *error_message = xstrdup (s);
        done = true;
      }
    else
      {
        assert (*ifile <= 1);
        *ifile = 1 - *ifile;

        FILE *f;
        open_file (filename[*ifile], "r", &f, error_message);
        done = (*error_message != NULL);
        if (!done)
          {
            FILE *g;
            open_file (filename[1 - *ifile], "w", &g, error_message);
            done = (*error_message != NULL);
            if (done)
              fclose (f);
            if (!done)
              {
                buffered_token_getter_t input_getter =
                  make_buffered_token_getter_from_serialized_tokens
                  (filename[*ifile], f);
                buffered_token_getter_t getter;
                const bool (*check_for_mismatch)
                  (buffered_token_getter_t, token_t);
                make_token_getter_with_mismatch_check
                  (input_getter, &getter, &check_for_mismatch);

                token_putter_t input_putter =
                  make_token_putter_to_stream_serialized_t
                  (filename[1 - *ifile], g);
                token_putter_t putter =
                  make_token_putter_with_mismatch_check
                  (input_putter, getter, check_for_mismatch);

                scan_tokens (NULL, getter, putter, error_message);
                done = (*error_message != NULL
                        || !check_for_mismatch (getter, NULL));

                fclose (f);
                fclose (g);

                ipass += 1;
              }
          }
      }
}

static void
get_token_stream_filenames (const char *filename[2],
                            const char *extension)
{
  char s[100];
  for (size_t i = 0; i != 2; i += 1)
    {
      snprintf (s, 100, "%zu%s", i, extension);
      size_t nwd = strlen (work_directory ());
      size_t ns = strlen (s);
      char *t = XCALLOC (nwd + ns + 2, char);
      memcpy (t, work_directory (), nwd * sizeof (char));
      t[nwd] = '/';
      memcpy (t + (nwd + 1), s, ns * sizeof (char));
      filename[i] = t;
    }
}

HIHA_VISIBLE void
scan_source_files_to_serialized_tokens (size_t n,
                                        const char *filenames[n],
                                        const char **tokens,
                                        const char **error_message)
{
  const char *fn[2];
  size_t ifile;
  buffered_token_getter_t getter;
  const bool (*check_for_mismatch) (buffered_token_getter_t, token_t);
  FILE *f;

  get_token_stream_filenames (fn, ".hiha-serialized-tokens");

  buffered_token_getter_t input_getter =
    make_buffered_token_getter_from_source_files (n, filenames);
  make_token_getter_with_mismatch_check (input_getter, &getter,
                                         &check_for_mismatch);

  ifile = 0;
  open_file (fn[ifile], "w", &f, error_message);
  if (*error_message == NULL)
    {
      token_putter_t input_putter =
        make_token_putter_to_stream_serialized_t (fn[ifile], f);
      token_putter_t putter =
        make_token_putter_with_mismatch_check (input_putter, getter,
                                               check_for_mismatch);
      scan_tokens (NULL, getter, putter, error_message);
      fclose (f);

      if (*error_message == NULL)
        {
          scan_serialized_tokens_until_fixed_point (fn, &ifile,
                                                    error_message);
          if (*error_message == NULL)
            *tokens = fn[ifile];
        }
    }
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
