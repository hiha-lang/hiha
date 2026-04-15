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
#include <string.h>
#include <stdatomic.h>
#include <error.h>
#include <xalloc.h>
#include <exitfail.h>
#include <base64.h>
#include <gl_avltree_list.h>
#include <gl_xlist.h>
#include <libhiha/token_t.h>
#include <libhiha/initialize_once.h>

// Change this if using gettext.
#define _(msgid) msgid

#define VISIBLE [[gnu::visibility ("default")]]

struct token_getter_from_source_file
{
  /* This struct must be castable to a struct token_getter. */

  void (*get_token) (token_getter_t this_struct,
                     token_t *tok, const char **error_message);

  const char *filename;
  FILE *f;
  char *buf;
  size_t nbuf;                  /* The size of the buf. */
  size_t n;                     /* How many are in the buf. */
  size_t line_no;               /* Starting at one. */
  size_t i_code_point;          /* Zero-based. */
  bool eof_reached;
};
static void get_token_from_source_file (token_getter_t, token_t *,
                                        const char **);

struct token_getter_from_serialized_tokens
{
  /* This struct must be castable to a struct token_getter. */

  void (*get_token) (token_getter_t this_struct,
                     token_t *tok, const char **error_message);

  const char *filename;
  FILE *f;
  char *buf;
  size_t nbuf;                  /* The size of the buf. */
  gl_list_t filenames;
  gl_list_t token_kinds;
  gl_list_t token_values;
};
static void get_token_from_serialized_tokens (token_getter_t, token_t *,
                                              const char **);

static token_t
make_token_t (string_t token_kind, string_t token_value,
              const char *filename, size_t line_no,
              size_t code_point_no)
{
  token_t tok = XMALLOC (struct token);
  tok->token_kind = token_kind;
  tok->token_value = token_value;
  tok->loc = XMALLOC (struct text_location);
  tok->loc->filename = filename;
  tok->loc->line_no = line_no;
  tok->loc->code_point_no = code_point_no;
  return tok;
}

VISIBLE token_getter_from_source_file_t
make_token_getter_from_source_file_t (const char *filename, FILE *f)
{
  token_getter_from_source_file_t getter =
    XMALLOC (struct token_getter_from_source_file);

  getter->get_token = &get_token_from_source_file;

  getter->filename = filename;
  getter->f = f;

  getter->nbuf = 1000;
  getter->buf = XNMALLOC (getter->nbuf, char);

  getter->n = 0;
  getter->line_no = 0;
  getter->i_code_point = 0;

  getter->eof_reached = false;

  return getter;
}

static void
fill_line_if_necessary (token_getter_from_source_file_t g)
{
  if (!g->eof_reached && g->i_code_point == g->n)
    {
      const ssize_t nread = getline (&g->buf, &g->nbuf, g->f);
      g->eof_reached = (nread == -1);
      if (!g->eof_reached)
        {
          g->n = nread;
          g->line_no += 1;
          g->i_code_point = 0;
        }
    }
}

static void
get_token_from_source_file (token_getter_t getter, token_t *tok,
                            const char **error_message)
{
  token_getter_from_source_file_t g =
    (token_getter_from_source_file_t) getter;
  *error_message = NULL;
  const bool was_end_of_line = (0 < g->line_no && g->n != 0
                                && g->buf[g->n - 1] == '\n');
  fill_line_if_necessary (g);
  if (!g->eof_reached)
    {
      string_t str = XMALLOC (struct string);
      str->s = XNMALLOC (1, uint32_t);
      str->s[0] = g->buf[g->i_code_point];
      str->n = 1;
      *tok =
        make_token_t (copy_string_t (string_t_CP ()), str, g->filename,
                      g->line_no, g->i_code_point + 1);
      g->i_code_point += 1;
    }
  else if (was_end_of_line)
    *tok =
      make_token_t (copy_string_t (string_t_EOF ()),
                    copy_string_t (string_t_EOF ()), g->filename,
                    g->line_no + 1, 0);
  else
    *tok =
      make_token_t (copy_string_t (string_t_EOF ()),
                    copy_string_t (string_t_EOF ()), g->filename,
                    g->line_no, g->i_code_point + 1);
}

static bool
str_equal (const void *s1, const void *s2)
{
  /* A string equality test that allows for NULL as a value. We want
     this because NULL is a possible ‘filename’. */

  bool b = false;
  if (s1 == NULL)
    b = (s2 == NULL);
  else if (s2 == NULL)
    b = false;
  else
    b = (0 == strcmp ((const char *) s1, (const char *) s2));
  return b;
}

static initialize_once_t _serialized_filenames_init1t =
  INITIALIZE_ONCE_T_INIT;
static gl_list_t _serialized_filenames = NULL;

static initialize_once_t _serialized_token_kind_init1t =
  INITIALIZE_ONCE_T_INIT;
static gl_list_t _serialized_token_kind = NULL;

static initialize_once_t _serialized_token_value_init1t =
  INITIALIZE_ONCE_T_INIT;
static gl_list_t _serialized_token_value = NULL;

static void
_initialize_serialized_filenames (void)
{
  _serialized_filenames =
    gl_list_create_empty (GL_AVLTREE_LIST, str_equal, NULL,
                          NULL, false);

  /* The NULL filename will be represented by the index 0. */
  gl_list_add_last (_serialized_filenames, NULL);
}

static gl_list_t
serialized_filenames (void)
{
  INITIALIZE_ONCE (_serialized_filenames_init1t,
                   _initialize_serialized_filenames);
  return _serialized_filenames;
}

static void
_initialize_serialized_token_kind (void)
{
  _serialized_token_kind =
    gl_list_create_empty (GL_AVLTREE_LIST, str_equal, NULL,
                          NULL, false);
}

static gl_list_t
serialized_token_kinds (void)
{
  INITIALIZE_ONCE (_serialized_token_kind_init1t,
                   _initialize_serialized_token_kind);
  return _serialized_token_kind;
}

static void
_initialize_serialized_token_value (void)
{
  _serialized_token_value =
    gl_list_create_empty (GL_AVLTREE_LIST, str_equal, NULL,
                          NULL, false);
}

static gl_list_t
serialized_token_values (void)
{
  INITIALIZE_ONCE (_serialized_token_value_init1t,
                   _initialize_serialized_token_value);
  return _serialized_token_value;
}

VISIBLE void
serialize_token_t (const token_t tok, FILE *f)
{
  /* The serialization done here is NOT interchangeable between
     machines. */

  gl_list_t fnames = serialized_filenames ();
  size_t i = gl_list_indexof (fnames, tok->loc->filename);
  if (i == (size_t) (-1))
    {
      gl_list_add_last (fnames, tok->loc->filename);
      i = gl_list_size (fnames) - 1;

      /* Write the filename in BASE64-encoding of the locale-encoded
         string, after a number that will be used to represent the
         filename. */
      idx_t inlen = strlen (tok->loc->filename);
      idx_t outlen = BASE64_LENGTH (inlen) + 1;
      char *buf = XNMALLOC (outlen, char);
      base64_encode (tok->loc->filename, inlen, buf, outlen);
      /* F number length filename */
      fprintf (f, "F %zu %zu %s\n", i, inlen, buf);
      free (buf);
    }

  size_t inlen1 = tok->token_kind->n * sizeof (uint32_t);
  size_t outlen1 = BASE64_LENGTH (inlen1) + 1;
  char *buf1 = XNMALLOC (outlen1, char);
  base64_encode ((const char *) tok->token_kind->s, inlen1, buf1,
                 outlen1);
  gl_list_t tokkinds = serialized_token_kinds ();
  size_t j = gl_list_indexof (tokkinds, buf1);
  if (j == (size_t) (-1))
    {
      gl_list_add_last (tokkinds, xstrdup (buf1));
      j = gl_list_size (tokkinds) - 1;

      /* Write the token_kind in BASE64-encoding of the UTF32-encoded
         string, in native byte order, after a number that will be
         used in place of the string. */
      fprintf (f, "K %zu %zu %s\n", j, tok->token_kind->n, buf1);
    }

  size_t inlen2 = tok->token_value->n * sizeof (uint32_t);
  size_t outlen2 = BASE64_LENGTH (inlen2) + 1;
  char *buf2 = XNMALLOC (outlen2, char);
  base64_encode ((const char *) tok->token_value->s, inlen2, buf2,
                 outlen2);
  gl_list_t tokvals = serialized_token_values ();
  size_t k = gl_list_indexof (tokvals, buf2);
  if (k == (size_t) (-1))
    {
      gl_list_add_last (tokvals, xstrdup (buf2));
      k = gl_list_size (tokvals) - 1;

      /* Write the token_value in BASE64-encoding of the UTF32-encoded
         string, in native byte order, after a number that will be
         used in place of the string. */
      fprintf (f, "V %zu %zu %s\n", k, tok->token_value->n, buf2);
    }

  fprintf (f, "T %zu %zu %zu %zu %zu\n", i, tok->loc->line_no,
           tok->loc->code_point_no, j, k);

  free (buf1);
  free (buf2);
}

VISIBLE token_getter_from_serialized_tokens_t
make_token_getter_from_serialized_tokens_t (const char *filename,
                                            FILE *f)
{
  token_getter_from_serialized_tokens_t getter =
    XMALLOC (struct token_getter_from_serialized_tokens);

  getter->get_token = &get_token_from_serialized_tokens;

  getter->filename = filename;
  getter->f = f;

  getter->nbuf = 1000;
  getter->buf = XNMALLOC (getter->nbuf, char);

  getter->filenames =
    gl_list_create_empty (GL_AVLTREE_LIST, NULL, NULL, NULL, true);
  /* The NULL filename will be represented by the index 0. */
  gl_list_add_last (getter->filenames, NULL);
  getter->token_kinds =
    gl_list_create_empty (GL_AVLTREE_LIST, NULL, NULL, NULL, true);
  getter->token_values =
    gl_list_create_empty (GL_AVLTREE_LIST, NULL, NULL, NULL, true);

  return getter;
}

static void
deserialize_filename (token_getter_from_serialized_tokens_t g,
                      ssize_t *nread)
{
  char F;
  size_t index;
  size_t string_size;
  char *b64;
  int retval =
    sscanf (g->buf, "%c %zu %zu %ms", &F, &index, &string_size, &b64);
  idx_t nb64 = BASE64_LENGTH (string_size);
  if (retval != 4 || F != 'F' || index != gl_list_size (g->filenames)
      || nb64 != strlen (b64))
    *nread = -101;
  else
    {
      char *s = XCALLOC (string_size + 1, char);
      idx_t outlen = string_size;
      base64_decode (b64, nb64, s, &outlen);
      free (b64);
      gl_list_add_last (g->filenames, s);
    }
}

static void
deserialize_token_kind (token_getter_from_serialized_tokens_t g,
                        ssize_t *nread)
{
  char K;
  size_t index;
  size_t string_size;
  char *b64;
  int retval =
    sscanf (g->buf, "%c %zu %zu %ms", &K, &index, &string_size, &b64);
  idx_t nb64 = BASE64_LENGTH (string_size * sizeof (uint32_t));
  if (retval != 4 || K != 'K' || index != gl_list_size (g->token_kinds)
      || nb64 != strlen (b64))
    *nread = -102;
  else
    {
      string_t s = XMALLOC (struct string);
      s->n = string_size;
      s->s = XNMALLOC (s->n, uint32_t);
      idx_t outlen = s->n * sizeof (uint32_t);
      base64_decode (b64, nb64, (char *) s->s, &outlen);
      free (b64);
      gl_list_add_last (g->token_kinds, s);
    }
}

static void
deserialize_token_value (token_getter_from_serialized_tokens_t g,
                         ssize_t *nread)
{
  char V;
  size_t index;
  size_t string_size;
  char *b64;
  int retval =
    sscanf (g->buf, "%c %zu %zu %ms", &V, &index, &string_size, &b64);
  idx_t nb64 = BASE64_LENGTH (string_size * sizeof (uint32_t));
  if (retval != 4 || V != 'V' || index != gl_list_size (g->token_values)
      || nb64 != strlen (b64))
    *nread = -103;
  else
    {
      string_t s = XMALLOC (struct string);
      s->n = string_size;
      s->s = XNMALLOC (s->n, uint32_t);
      idx_t outlen = s->n * sizeof (uint32_t);
      base64_decode (b64, nb64, (char *) s->s, &outlen);
      free (b64);
      gl_list_add_last (g->token_values, s);
    }
}

static void
deserialize_token (token_getter_from_serialized_tokens_t g,
                   token_t *tok, ssize_t *nread)
{
  char T;
  size_t i_filename;
  size_t line_no;
  size_t code_point_no;
  size_t i_token_kind;
  size_t i_token_value;

  int retval =
    sscanf (g->buf, "%c %zu %zu %zu %zu %zu", &T, &i_filename, &line_no,
            &code_point_no, &i_token_kind, &i_token_value);
  if (retval != 6 || T != 'T'
      || gl_list_size (g->filenames) <= i_filename
      || gl_list_size (g->token_kinds) <= i_token_kind
      || gl_list_size (g->token_values) <= i_token_value)
    *nread = -104;
  else
    {
      const char *filename = gl_list_get_at (g->filenames, i_filename);
      string_t tokkind =
        copy_string_t ((string_t) gl_list_get_at (g->token_kinds,
                                                  i_token_kind));
      string_t tokvalue =
        copy_string_t ((string_t) gl_list_get_at (g->token_values,
                                                  i_token_value));
      *tok =
        make_token_t (tokkind, tokvalue, filename, line_no,
                      code_point_no);
    }
}


static void
make_serialized_line_available (token_getter_from_serialized_tokens_t g,
                                token_t *tok, ssize_t *nread)
{
  if (0 <= *nread && *tok == NULL)
    *nread = getline (&g->buf, &g->nbuf, g->f);
}

static void
get_token_from_serialized_tokens (token_getter_t getter, token_t *tok,
                                  const char **error_message)
{
  token_getter_from_serialized_tokens_t g =
    (token_getter_from_serialized_tokens_t) getter;
  *error_message = NULL;

  size_t nread = 0;
  *tok = NULL;
  make_serialized_line_available (g, tok, &nread);
  while (0 <= nread && *tok == NULL)
    {
      switch (g->buf[0])
        {
        case 'F':
          deserialize_filename (g, &nread);
          break;
        case 'K':
          deserialize_token_kind (g, &nread);
          break;
        case 'V':
          deserialize_token_value (g, &nread);
          break;
        case 'T':
          deserialize_token (g, tok, &nread);
          break;
        default:
          nread = -100;
          break;
        }
      make_serialized_line_available (g, tok, &nread);
    }
  if (*tok == NULL)
    {
      char s[1000];
      snprintf (s, 1000, _("error involving serialized tokens %s"),
                g->filename);
      *error_message = xstrdup (s);
    }
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
