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
#include <errno.h>
#include <error.h>
#include <exitfail.h>
#include <quotearg.h>
#include <libhiha/libhiha.h>

#define _(msgid) HIHA_GETTEXT (msgid)

static string_t str_CP;
static string_t str_EOF;
static string_t str_FSEP;
static string_t str_SP;
static string_t str_CO;
static string_t str_SY;
static string_t str_ID;
static string_t str_I10;
static string_t str_I_I10;
static string_t str_F10;
static string_t str_R10;
static string_t str_KW;

static string_t strings[13];

///////nud_handler_t next_cp_handler;
///////nud_handler_t next_eof_handler;
///////nud_handler_t next_fsep_handler;
///////nud_handler_t next_sp_handler;
///////nud_handler_t next_co_handler;
///////nud_handler_t next_sy_handler;
///////nud_handler_t next_id_handler;
///////nud_handler_t next_i10_handler;
///////nud_handler_t next_i_i10_handler;
///////nud_handler_t next_f10_handler;
///////nud_handler_t next_r10_handler;
///////nud_handler_t next_kw_handler;

static void
initialize_strings (void)
{
  str_CP = string_t_CP ();
  str_EOF = string_t_EOF ();
  str_FSEP = make_string_t ("FSEP");
  str_SP = make_string_t ("SP");
  str_CO = make_string_t ("CO");
  str_SY = make_string_t ("SY");
  str_ID = make_string_t ("ID");
  str_I10 = make_string_t ("I10");
  str_I_I10 = make_string_t ("I.I10");
  str_F10 = make_string_t ("F10");
  str_R10 = make_string_t ("R10");
  str_KW = make_string_t ("KW");

  strings[0] = str_CP;
  strings[1] = str_EOF;
  strings[2] = str_FSEP;
  strings[3] = str_SP;
  strings[4] = str_CO;
  strings[5] = str_SY;
  strings[6] = str_ID;
  strings[7] = str_I10;
  strings[8] = str_I_I10;
  strings[9] = str_F10;
  strings[10] = str_R10;
  strings[11] = str_KW;
  strings[12] = NULL;
}

static const char *
quoted_str_nul (const char *s)
{
  struct quoting_options *qu_opts = clone_quoting_options (NULL);
  set_quoting_style (qu_opts, c_quoting_style);
  return quotearg_alloc (s, strlen (s) + 1, qu_opts);
}

static const char *
quoted_string_t (string_t str)
{
  size_t n_u8;
  uint8_t *u8 = u32_to_u8 (str->s, str->n, NULL, &n_u8);
  int err_number = errno;
  if (u8 == NULL)
    {
      error (exit_failure, err_number, "%s", make_str_nul (str));
      abort ();
    }
  struct quoting_options *qu_opts = clone_quoting_options (NULL);
  set_quoting_style (qu_opts, c_quoting_style);
  return quotearg_alloc ((const char *) u8, n_u8, qu_opts);
}

static void
dump_token (token_t tok)
{
  fprintf (stderr, "%-*s %*zu %*zu   %-*s %s\n",
           20, quoted_str_nul (tok->loc->filename),
           5, tok->loc->line_no, 5, tok->loc->code_point_no,
           5, make_str_nul (tok->token_kind),
           quoted_string_t (tok->token_value));
  fflush (stderr);
}

static void
handler (void *state, buffered_token_getter_t getter,
         pratt_tables_t tables, token_t tok,
         token_t *lhs, const char **error_message)
{
  if (*error_message == NULL)
    {
      dump_token (tok);
      *lhs = tok;
    }
}

HIHA_VISIBLE void
plugin_init (void)
{
  initialize_strings ();

  pratt_tables_t tables;

  acquire_pratt_tables_lock ();

  tables = get_pratt_tables_for_pass ("600-dump-tokens");
  for (string_t *p = strings; *p != NULL; p += 1)
    pratt_nud_put (tables, *p, &handler);
  set_pratt_tables_for_pass ("600-dump-tokens", tables);

  release_pratt_tables_lock ();
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
