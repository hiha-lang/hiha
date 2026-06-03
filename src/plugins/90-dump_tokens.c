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
  if (dump_tokens_stream != NULL)
    {
      fprintf (dump_tokens_stream, "%*s %*zu %*zu   %*s %s\n",
               dump_tokens_widths[0],
               quoted_str_nul (tok->loc->filename),
               dump_tokens_widths[1], tok->loc->line_no,
               dump_tokens_widths[2], tok->loc->code_point_no,
               dump_tokens_widths[3], make_str_nul (tok->token_kind),
               quoted_string_t (tok->token_value));
      fflush (dump_tokens_stream);
    }
}

nud_handler_t next_handler;

static void
handler (void *state, buffered_token_getter_t getter,
         pratt_tables_t tables, token_t tok,
         token_t *lhs, const char **error_message)
{
  if (*error_message == NULL)
    {
      dump_token (tok);
      next_handler (state, getter, tables, tok, lhs, error_message);
    }
}

HIHA_VISIBLE void
plugin_init (void)
{
  pratt_tables_t tables;

  acquire_pratt_tables_lock ();

  tables = get_pratt_tables_for_pass ("1000-dump-tokens");
  next_handler = pratt_nud_get_default (tables);
  pratt_nud_put_default (tables, &handler);
  set_pratt_tables_for_pass ("1000-dump-tokens", tables);

  release_pratt_tables_lock ();
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
