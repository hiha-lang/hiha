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
#include <libhiha/initialize_once.h>

// Change this if using gettext.
#define _(msgid) msgid

#define VISIBLE [[gnu::visibility ("default")]]

static initialize_once_t _string_constants_init1t =
  INITIALIZE_ONCE_T_INIT;
static string_t _string_t_EOF;
static string_t _string_t_CP;

static void
_initialize_string_constants (void)
{
  _string_t_EOF = make_string_t ("EOF");
  _string_t_CP = make_string_t ("CP");
}

VISIBLE const string_t
string_t_EOF (void)
{
  INITIALIZE_ONCE (_string_constants_init1t,
		   _initialize_string_constants);
  return _string_t_EOF;
}

VISIBLE const string_t
string_t_CP (void)
{
  INITIALIZE_ONCE (_string_constants_init1t,
		   _initialize_string_constants);
  return _string_t_CP;
}

VISIBLE int
string_t_cmp (const string_t str1, const string_t str2)
{
  return u32_cmp2 (str1->s, str1->n, str2->s, str2->n);
}

VISIBLE string_t
make_string_t (const char *src)
{
  return string_t_canonical_from_str_len (src, strlen (src), NULL);
}

VISIBLE string_t
copy_string_t (const string_t str)
{
  string_t s = XMALLOC (struct string);
  s->n = str->n;
  s->s = XNMALLOC (s->n, uint32_t);
  memcpy (s->s, ((const struct string *) str)->s,
	  s->n * sizeof (uint32_t));
  return s;
}

VISIBLE char *
make_str_nul (const string_t str)
{
  char *s;
  size_t n;
  str_len_from_string_t (str, &s, &n);
  char *t = XNMALLOC (n + 1, char);
  memcpy (t, s, n * sizeof (char));
  t[n] = '\0';
  return t;
}

VISIBLE string_t
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
      /* It is idiom to call abort(3) after error(exit_failure,...),
         to obtain the [[noreturn]] qualifier of abort(3). The
         abort(3) is actually never reached. */
      abort ();
    }
  string_t result = XMALLOC (struct string);
  result->s = u32;
  result->n = n;
  return result;
}

VISIBLE string_t
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

VISIBLE string_t
string_t_canonical_from_str_len (const char *src, size_t srclen,
				 text_location_t loc)
{
  string_t str1 = string_t_from_str_len (src, srclen, loc);
  string_t str = string_t_canonicalize (str1, loc);
  return str;
}

VISIBLE void
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

VISIBLE char *
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

VISIBLE void
print_string_t (const string_t str, FILE *f)
{
  char *s;
  size_t n;
  str_len_from_string_t (str, &s, &n);
  fwrite (s, sizeof (char), n, f);
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
