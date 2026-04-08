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
#include <string.h>
#include <errno.h>
#include <error.h>
#include <xalloc.h>
#include <exitfail.h>
#include <libhiha/string_t.h>

// Change this if using gettext.
#define _(msgid) msgid

void
string_t_free (string_t str)
{
  if (str != NULL)
    {
      free (str->s);
      free (str);
    }
}

int
string_t_cmp (const string_t str1, const string_t str2)
{
  return u32_cmp2 (str1->s, str1->n, str2->s, str2->n);
}

string_t
make_string_t (const char *src)
{
  return string_t_from_str_len (src, strlen (src), NULL);
}

char *
make_str_nul (const string_t str)
{
  char *s;
  size_t n;
  str_len_from_string_t (str, &s, &n);
  s = xrealloc (s, n + 1);
  s[n] = '\0';
  return s;
}

string_t
string_t_from_str_len (const char *src, size_t srclen,
		       text_location_t loc)
{
  size_t n;
  uint32_t *u32 = u32_conv_from_encoding (locale_charset (),
					  iconveh_replacement_character,
					  src, srclen, NULL,
					  NULL, &n);
  int err_number = errno;
  if (u32 == NULL)
    {
      char *locstr = text_location_string (loc);
      error (exit_failure, err_number, "%s", locstr);
      abort ();
    }
  string_t result = XMALLOC (struct string);
  result->s = u32;
  result->n = n;
  return result;
}

string_t
string_t_canonicalize (const string_t src, text_location_t loc)
{
  size_t n;
  uint32_t *u32 = u32_normalize (UNINORM_NFC, src->s, src->n,
				 NULL, &n);
  int err_number = errno;
  if (u32 == NULL)
    {
      char *locstr = text_location_string (loc);
      error (exit_failure, err_number, "%s", locstr);
      abort ();
    }
  string_t result = XMALLOC (struct string);
  result->s = u32;
  result->n = n;
  return result;
}

string_t
string_t_canonical_from_str_len (const char *src, size_t srclen,
				 text_location_t loc)
{
  string_t str1 = string_t_from_str_len (src, srclen, loc);
  string_t str = string_t_canonicalize (str1, loc);
  string_t_free (str1);
  return str;
}

void
str_len_from_string_t (const string_t src, char **s, size_t *n)
{
  *s = u32_conv_to_encoding (locale_charset (),
			     iconveh_replacement_character, src->s,
			     src->n, NULL, NULL, n);
  int err_number = errno;
  if (*s == NULL)
    {
      error (exit_failure, err_number, "");
      abort ();
    }
}

void
text_location_t_free (text_location_t loc)
{
  free (loc);
}

char *
text_location_string (text_location_t loc)
{
  const char *fn;
  if (loc == NULL || loc->filename == NULL)
    fn = _("〈no·filename〉");
  else
    fn = loc->filename;
  size_t fn_len = strlen (fn);

  char ln[100];
  if (loc == NULL || loc->line_no == 0)
    ln[0] = '\0';
  else
    snprintf (ln, 100, _(", line %zu"), loc->line_no);
  size_t ln_len = strlen (ln);

  char *s = XCALLOC (fn_len + ln_len + 1, char);
  memcpy (s, fn, fn_len * sizeof (char));
  memcpy (s + (fn_len * sizeof (char)), ln, ln_len * sizeof (char));

  return s;
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
