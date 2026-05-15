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
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <xalloc.h>
#include <exitfail.h>
#include <gl_xlist.h>
#include <gl_avltree_list.h>
#include <libhiha/string_t.h>
#include <libhiha/initialize_once.h>

#define _(msgid) HIHA_GETTEXT (msgid)

static initialize_once_t _string_constants_init1t =
  INITIALIZE_ONCE_T_INIT;
static string_t _string_t_EOF;
static string_t _string_t_CP;
static string_t _string_t_formfeed;

static void
_initialize_string_constants (void)
{
  _string_t_EOF = make_string_t ("EOF");
  _string_t_CP = make_string_t ("CP");
  _string_t_formfeed = make_string_t ("\014");
}

HIHA_VISIBLE HIHA_PURE const string_t
string_t_EOF (void)
{
  INITIALIZE_ONCE (_string_constants_init1t,
                   _initialize_string_constants);
  return _string_t_EOF;
}

HIHA_VISIBLE HIHA_PURE const string_t
string_t_CP (void)
{
  INITIALIZE_ONCE (_string_constants_init1t,
                   _initialize_string_constants);
  return _string_t_CP;
}

HIHA_VISIBLE HIHA_PURE const string_t
string_t_formfeed (void)
{
  INITIALIZE_ONCE (_string_constants_init1t,
                   _initialize_string_constants);
  return _string_t_formfeed;
}

HIHA_VISIBLE int
string_t_cmp (const string_t str1, const string_t str2)
{
  return u32_cmp2 (str1->s, str1->n, str2->s, str2->n);
}

HIHA_VISIBLE string_t
make_string_t (const char *src)
{
  return string_t_canonical_from_str_len (src, strlen (src), NULL);
}

HIHA_VISIBLE string_t
copy_string_t (const string_t str)
{
  string_t s = XMALLOC (struct string);
  s->n = str->n;
  s->s = XNMALLOC (s->n, uint32_t);
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

  gl_list_t lst =
    gl_list_create_empty (GL_AVLTREE_LIST, NULL, NULL, NULL, true);
  string_t str = va_arg (args, string_t);
  while (str != NULL)
    {
      gl_list_add_last (lst, str);
      str = va_arg (args, string_t);
    }

  string_t result = XMALLOC (struct string);

  result->n = 0;
  for (i = 0; i != gl_list_size (lst); i += 1)
    {
      str = (string_t) gl_list_get_at (lst, i);
      result->n += str->n;
    }
  result->s = XNMALLOC (result->n, uint32_t);

  j = 0;
  for (i = 0; i != gl_list_size (lst); i += 1)
    {
      str = (string_t) gl_list_get_at (lst, i);
      memcpy (result->s + j, str->s, str->n * sizeof (uint32_t));
      j += str->n;
    }

  va_end (args);

  return result;
}

HIHA_VISIBLE char *
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

HIHA_VISIBLE string_t
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

HIHA_VISIBLE string_t
string_t_canonical_from_str_len (const char *src, size_t srclen,
                                 text_location_t loc)
{
  string_t str1 = string_t_from_str_len (src, srclen, loc);
  string_t str = string_t_canonicalize (str1, loc);
  return str;
}

HIHA_VISIBLE void
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

  char *s = XCALLOC (fn_len + ln_len + 1, char);
  memcpy (s, fn, fn_len * sizeof (char));
  memcpy (s + (fn_len * sizeof (char)), ln, ln_len * sizeof (char));

  return s;
}

HIHA_VISIBLE void
print_string_t (const string_t str, FILE *f)
{
  char *s;
  size_t n;
  str_len_from_string_t (str, &s, &n);
  fwrite (s, sizeof (char), n, f);
}

/*--------------------------------------------------------------------*/

struct string_t_hash_context
{
  string_t str;
  uint64_t seeds[2];
  uint64_t *hashes;             /* Memoized hashes. */
  size_t num_hashes;
};

static const void *const string_t_hash_unique_address = "";
static const void *const string_t_hash_this_first[1] = {
  [0] = string_t_hash_unique_address
};

HIHA_VISIBLE string_t_hash_context_t
string_t_hash_init (string_t str)
{
  string_t_hash_context_t context =
    XMALLOC (struct string_t_hash_context);
  context->str = str;
  context->hashes = NULL;
  context->num_hashes = 0;
  return context;
}

static inline void
check_string_t_hash_index (unsigned int i)
{
  /* Put this here to make the program “terminating in principle” when
     things like ideal hashmaps do the practically impossible. */
  if (i == ((unsigned int) INT_MAX) + 1)
    {
      error (exit_failure, 0,
             "string_t_hash index out of bounds: %u", i);
      abort ();
    }
}

static void
copy_old_string_t_hashes (uint64_t *new_hashes,
                          string_t_hash_context_t context)
{
  if (context->hashes != NULL)
    memcpy (new_hashes, context->hashes,
            context->num_hashes * sizeof (uint64_t));
}

static void
compute_new_string_t_hashes (uint64_t *new_hashes,
                             size_t new_num_hashes,
                             string_t_hash_context_t context)
{
  /* Memoize all the hashes up to new_num_hashes.

     Memoizing all the hashes (rather than doing anything more
     complicated) presumes algorithms will go progressively from hash
     0, to hash 1, to hash 2, and so on. Thus, in practice, there will
     be no hashes computed and simply thrown away. */

  spookyhash_context_t boo;

  for (size_t i = context->num_hashes; i != new_num_hashes; i += 2)
    {
      uint64_t seed1 = i / 2;
      uint64_t seed2 = seed1;
      spookyhash_init (&boo, seed1, seed2);
      spookyhash_update (&boo, string_t_hash_this_first,
                         sizeof (const void *const));
      spookyhash_update (&boo, context->str->s,
                         context->str->n * sizeof (uint32_t));
      spookyhash_final (&boo, &new_hashes[i], &new_hashes[i + 1]);
    }
}

HIHA_VISIBLE uint64_t
string_t_hash (string_t_hash_context_t context, unsigned int i)
{
  check_string_t_hash_index (i);

  /* j = i rounded to the next higher even, if it originally be
     odd. Otherwise kept the same. */
  unsigned int j = (i + 1) & (~1U);

  if (context->num_hashes < j)
    {
      uint64_t *new_hashes = XNMALLOC (j, uint64_t);
      copy_old_string_t_hashes (new_hashes, context);
      compute_new_string_t_hashes (new_hashes, j, context);
      context->hashes = new_hashes;
      context->num_hashes = j;
    }

  return context->hashes[i];
}

/*--------------------------------------------------------------------*/
/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
