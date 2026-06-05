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
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <xalloc.h>
#include <exitfail.h>
#include <libhiha/string_t.h>
#include <libhiha/xalloc.h>
#include <libhiha/initialize_once.h>

#define _(msgid) HIHA_GETTEXT (msgid)

static initialize_once_t _string_constants_init1t =
  INITIALIZE_ONCE_T_INIT;
static string_t _empty_string_t;
static string_t _string_t_EOF;
static string_t _string_t_CP;
static string_t _string_t_KW;
static string_t _string_t_OP;
static string_t _string_t_space;
static string_t _string_t_tab;
static string_t _string_t_newline;
static string_t _string_t_formfeed;
static string_t _string_t_zerowidth;

static void
_initialize_string_constants (void)
{
  _empty_string_t = make_string_t ("");
  _string_t_EOF = make_string_t ("EOF");
  _string_t_CP = make_string_t ("CP");
  _string_t_KW = make_string_t ("KW");
  _string_t_OP = make_string_t ("OP");
  _string_t_space = make_string_t (" ");
  _string_t_tab = make_string_t ("\t");
  _string_t_newline = make_string_t ("\n");
  _string_t_formfeed = make_string_t ("\014");
  _string_t_zerowidth = make_string_t ("\342\200\213");
}

HIHA_VISIBLE HIHA_PURE string_t
empty_string_t (void)
{
  INITIALIZE_ONCE (_string_constants_init1t,
                   _initialize_string_constants);
  return _empty_string_t;
}

HIHA_VISIBLE HIHA_PURE string_t
string_t_EOF (void)
{
  INITIALIZE_ONCE (_string_constants_init1t,
                   _initialize_string_constants);
  return _string_t_EOF;
}

HIHA_VISIBLE HIHA_PURE string_t
string_t_CP (void)
{
  INITIALIZE_ONCE (_string_constants_init1t,
                   _initialize_string_constants);
  return _string_t_CP;
}

HIHA_VISIBLE HIHA_PURE string_t
string_t_KW (void)
{
  INITIALIZE_ONCE (_string_constants_init1t,
                   _initialize_string_constants);
  return _string_t_KW;
}

HIHA_VISIBLE HIHA_PURE string_t
string_t_OP (void)
{
  INITIALIZE_ONCE (_string_constants_init1t,
                   _initialize_string_constants);
  return _string_t_OP;
}

HIHA_VISIBLE HIHA_PURE string_t
string_t_space (void)
{
  INITIALIZE_ONCE (_string_constants_init1t,
                   _initialize_string_constants);
  return _string_t_space;
}

HIHA_VISIBLE HIHA_PURE string_t
string_t_tab (void)
{
  INITIALIZE_ONCE (_string_constants_init1t,
                   _initialize_string_constants);
  return _string_t_tab;
}

HIHA_VISIBLE HIHA_PURE string_t
string_t_newline (void)
{
  INITIALIZE_ONCE (_string_constants_init1t,
                   _initialize_string_constants);
  return _string_t_newline;
}

HIHA_VISIBLE HIHA_PURE string_t
string_t_formfeed (void)
{
  INITIALIZE_ONCE (_string_constants_init1t,
                   _initialize_string_constants);
  return _string_t_formfeed;
}

HIHA_VISIBLE HIHA_PURE string_t
string_t_zerowidth (void)
{
  INITIALIZE_ONCE (_string_constants_init1t,
                   _initialize_string_constants);
  return _string_t_zerowidth;
}

HIHA_VISIBLE int
string_t_cmp (string_t str1, string_t str2)
{
  return u32_cmp2 (str1->s, str1->n, str2->s, str2->n);
}

HIHA_VISIBLE string_t
make_string_t (const char *src)
{
  return string_t_canonical_from_str_len (src, strlen (src), NULL);
}

HIHA_VISIBLE string_t
copy_string_t (string_t str)
{
  struct string *s = XMALLOC (struct string);
  s->n = str->n;
  s->s = XNMALLOC_ATOMIC (s->n, uint32_t);
  memcpy (s->s, ((const struct string *) str)->s,
          s->n * sizeof (uint32_t));
  return s;
}

HIHA_VISIBLE string_t
concat_string_t (...)
{
  va_list args;
  size_t i;
  size_t j;

  va_start (args);

  string_t_vector_t lst = NULL;
  string_t str = va_arg (args, string_t);
  while (str != NULL)
    {
      lst = string_t_vector_push (lst, str);
      str = va_arg (args, string_t);
    }

  struct string *result = XMALLOC (struct string);

  result->n = 0;
  for (i = 0; i != string_t_vector_length (lst); i += 1)
    {
      str = string_t_vector_ref (lst, i);
      result->n += str->n;
    }
  result->s = XNMALLOC_ATOMIC (result->n, uint32_t);

  j = 0;
  for (i = 0; i != string_t_vector_length (lst); i += 1)
    {
      str = string_t_vector_ref (lst, i);
      memcpy (result->s + j, str->s, str->n * sizeof (uint32_t));
      j += str->n;
    }

  va_end (args);

  return result;
}

HIHA_VISIBLE char *
make_str_nul (string_t str)
{
  char *s;
  size_t n;
  str_len_from_string_t (str, &s, &n);
  char *t = XNMALLOC_ATOMIC (n + 1, char);
  memcpy (t, s, n * sizeof (char));
  t[n] = '\0';
  return t;
}

HIHA_VISIBLE string_t
string_t_from_str_len (const char *src, size_t srclen,
                       text_location_t loc)
{
  size_t n;
  uint32_t *u32 = u32_conv_from_encoding (locale_charset (),
                                          iconveh_replacement_character,
                                          src, srclen, NULL,
                                          NULL, &n);
  int err_number = errno;

  if (u32 != NULL)
    {
      /* Reallocate as atomic storage. */
      uint32_t *u32_new = XNMALLOC_ATOMIC (n, uint32_t);
      memcpy (u32_new, u32, n * sizeof (uint32_t));
      u32 = u32_new;
    }
  else if (err_number == 0)
    {
      /* An empty string requires no storage, but we want storage
         anyway. Allocate an array of length one. */
      u32 = XNMALLOC_ATOMIC (1, uint32_t);
      u32[0] = 0;
    }
  else
    {
      char *locstr = text_location_string (loc);
      error (exit_failure, err_number, "%s", locstr);
      /* It is idiom to call abort(3) after error(exit_failure,...),
         to obtain the [[noreturn]] qualifier of abort(3). The
         abort(3) is actually never reached. */
      abort ();
    }
  struct string *result = XMALLOC (struct string);
  result->s = u32;
  result->n = n;
  return result;
}

HIHA_VISIBLE string_t
string_t_canonicalize (string_t src, text_location_t loc)
{
  size_t n;
  uint32_t *u32 = u32_normalize (UNINORM_NFC, src->s, src->n,
                                 NULL, &n);
  int err_number = errno;
  if (u32 != NULL)
    {
      /* Reallocate as atomic storage. */
      uint32_t *u32_new = XNMALLOC_ATOMIC (n, uint32_t);
      memcpy (u32_new, u32, n * sizeof (uint32_t));
      u32 = u32_new;
    }
  else if (err_number == 0)
    {
      /* An empty string requires no storage, but we want storage
         anyway. Allocate an array of length one. */
      u32 = XNMALLOC_ATOMIC (1, uint32_t);
      u32[0] = 0;
    }
  else
    {
      char *locstr = text_location_string (loc);
      error (exit_failure, err_number, "%s", locstr);
      abort ();
    }
  struct string *result = XMALLOC (struct string);
  result->s = u32;
  result->n = n;
  return result;
}

HIHA_VISIBLE string_t
string_t_canonical_from_str_len (const char *src, size_t srclen,
                                 text_location_t loc)
{
  string_t str1 = string_t_from_str_len (src, srclen, loc);
  string_t str = string_t_canonicalize (str1, loc);
  return str;
}

HIHA_VISIBLE void
str_len_from_string_t (string_t src, char **s, size_t *n)
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

HIHA_VISIBLE char *
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

  char cp[100];
  if (loc == NULL || loc->code_point_no == 0)
    cp[0] = '\0';
  else
    snprintf (cp, 100, _(", code point %zu"), loc->code_point_no);
  size_t cp_len = strlen (cp);

  char *s = XCALLOC_ATOMIC (fn_len + ln_len + cp_len + 1, char);
  memcpy (s, fn, fn_len * sizeof (char));
  memcpy (s + fn_len, ln, ln_len * sizeof (char));
  memcpy (s + fn_len + ln_len, cp, cp_len * sizeof (char));

  return s;
}

HIHA_VISIBLE void
print_string_t (string_t str, FILE *f)
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
