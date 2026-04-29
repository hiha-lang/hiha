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
#include <string.h>
#include <errno.h>
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

//
// It might be prudent to be prepared to copy strings, if this code
// ever becomes converted somehow to linear types instead of garbage
// collection.
//
#define NEW_STRING
//#define NEW_STRING copy_string_t

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
  string_t sbuf;                /* The UTF-32 encoding of buf. */
  size_t line_no;               /* Starting at one. */
  size_t i_code_point;          /* Zero-based. */
  bool eof_reached;
};
typedef struct token_getter_from_source_file
  *token_getter_from_source_file_t;
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
typedef struct token_getter_from_serialized_tokens
  *token_getter_from_serialized_tokens_t;
static void get_token_from_serialized_tokens (token_getter_t, token_t *,
                                              const char **);

VISIBLE token_t
make_token_t (string_t token_kind, string_t token_value,
              text_location_t loc)
{
  token_t tok = XMALLOC (struct token);
  tok->token_kind = token_kind;
  tok->token_value = token_value;
  tok->loc = loc;
  return tok;
}

static token_t
_make_token_t (string_t token_kind, string_t token_value,
               const char *filename, size_t line_no,
               size_t code_point_no)
{
  text_location_t loc = XMALLOC (struct text_location);
  loc->filename = filename;
  loc->line_no = line_no;
  loc->code_point_no = code_point_no;
  return make_token_t (token_kind, token_value, loc);
}

//VISIBLE token_getter_from_source_file_t
VISIBLE token_getter_t
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

  getter->sbuf = NULL;

  getter->line_no = 0;
  getter->i_code_point = 0;

  getter->eof_reached = false;

  return (token_getter_t) getter;
}

static void
fill_line_if_necessary (token_getter_from_source_file_t g)
{
  if (!g->eof_reached
      && (g->sbuf == NULL || g->i_code_point == g->sbuf->n))
    {
      const ssize_t nread = getline (&g->buf, &g->nbuf, g->f);
      g->eof_reached = (nread == -1);
      if (!g->eof_reached)
        {
          g->n = nread;
          text_location_t loc = XMALLOC (struct text_location);
          loc->filename = g->filename;
          loc->line_no = g->line_no + 1;
          loc->code_point_no = 0;
          g->sbuf = string_t_canonical_from_str_len (g->buf, g->n, loc);
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
  const bool was_end_of_line = (0 < g->line_no && g->sbuf->n != 0
                                && g->sbuf->s[g->sbuf->n - 1] == '\n');
  fill_line_if_necessary (g);
  if (!g->eof_reached)
    {
      string_t str = XMALLOC (struct string);
      str->s = XNMALLOC (1, uint32_t);
      str->s[0] = g->sbuf->s[g->i_code_point];
      str->n = 1;
      *tok =
        _make_token_t (NEW_STRING (string_t_CP ()), str, g->filename,
                       g->line_no, g->i_code_point + 1);
      g->i_code_point += 1;
    }
  else if (was_end_of_line)
    *tok =
      _make_token_t (NEW_STRING (string_t_EOF ()),
                     NEW_STRING (string_t_EOF ()), g->filename,
                     g->line_no + 1, 0);
  else
    *tok =
      _make_token_t (NEW_STRING (string_t_EOF ()),
                     NEW_STRING (string_t_EOF ()), g->filename,
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

VISIBLE token_getter_t
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

  return (token_getter_t) getter;
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
        NEW_STRING ((string_t) gl_list_get_at (g->token_kinds,
                                               i_token_kind));
      string_t tokvalue =
        NEW_STRING ((string_t) gl_list_get_at (g->token_values,
                                               i_token_value));
      *tok =
        _make_token_t (tokkind, tokvalue, filename, line_no,
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

  ssize_t nread = 0;
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

struct _buffered_token_getter
{
  void (*get_token) (buffered_token_getter_t this_struct,
                     token_t *tok, const char **error_message);
  void (*look_at_token) (buffered_token_getter_t this_struct, size_t,
                         token_t *tok, const char **error_message);
  token_getter_t getter;
  gl_list_t buffer;
};
typedef struct _buffered_token_getter *_buffered_token_getter_t;

static void
get_token_from_buffered_getter (buffered_token_getter_t getter,
                                token_t *tok,
                                const char **error_message)
{
  _buffered_token_getter_t g = (_buffered_token_getter_t) getter;
  *error_message = NULL;

  if (gl_list_size (g->buffer) != 0)
    {
      *tok = (token_t) gl_list_get_first (g->buffer);
      gl_list_remove_first (g->buffer);
    }
  else
    g->getter->get_token (g->getter, tok, error_message);
}

static void
look_at_buffered_token (buffered_token_getter_t getter, size_t i,
                        token_t *tok, const char **error_message)
{
  _buffered_token_getter_t g = (_buffered_token_getter_t) getter;
  *tok = NULL;
  *error_message = NULL;

  while (*error_message == NULL && gl_list_size (g->buffer) <= i)
    {
      token_t t;
      g->getter->get_token (g->getter, &t, error_message);
      if (*error_message == NULL)
        gl_list_add_last (g->buffer, t);
    }
  if (*error_message == NULL)
    *tok = (token_t) gl_list_get_at (g->buffer, i);
}

VISIBLE buffered_token_getter_t
make_buffered_token_getter_t (token_getter_t unbuffered_getter)
{
  _buffered_token_getter_t g = XMALLOC (struct _buffered_token_getter);
  g->getter = unbuffered_getter;
  g->buffer =
    gl_list_create_empty (GL_AVLTREE_LIST, NULL, NULL, NULL, true);
  g->get_token = &get_token_from_buffered_getter;
  g->look_at_token = &look_at_buffered_token;
  return (buffered_token_getter_t) g;
}

struct _multiple_files_getter
{
  /* This struct must be castable to a struct token_getter. */

  void (*get_token) (token_getter_t this_struct,
                     token_t *tok, const char **error_message);

  const char **filenames;
  size_t n;
  ssize_t i;
  FILE *f;
  token_getter_t getter;
};
typedef struct _multiple_files_getter *_multiple_files_getter_t;

static void
open_one_of_multiple_files (_multiple_files_getter_t g)
{
  g->f = fopen (g->filenames[g->i], "r");
  if (g->f == NULL)
    {
      int err_number = errno;
      error (exit_failure, err_number, "%s", g->filenames[g->i]);
      abort ();
    }
  g->getter =
    make_token_getter_from_source_file_t (g->filenames[g->i], g->f);
}

static void
get_token_from_multiple_files_getter (token_getter_t getter,
                                      token_t *tok,
                                      const char **error_message)
{
  //
  // Play the files in sequence, converting all the EOF but the last
  // to formfeed.
  //

  _multiple_files_getter_t g = (_multiple_files_getter_t) getter;

  assert (-1 <= g->i);
  assert (g->i + 1 <= g->n + 1);

  *error_message = NULL;

  if (g->n == 0 || g->i == g->n)
    *tok = make_token_t (NEW_STRING (string_t_EOF ()),
                         NEW_STRING (string_t_EOF ()), NULL);
  else
    {
      if (g->i == -1)
        {
          // Open the first file.
          g->i += 1;
          open_one_of_multiple_files (g);
        }

      g->getter->get_token (g->getter, tok, error_message);

      if (*error_message == NULL)
        {
          if (string_t_cmp ((*tok)->token_kind, string_t_EOF ()) == 0)
            {
              // Close the finished file.
              fclose (g->f);

              g->i += 1;
              if (g->i != g->n)
                {
                  // There is at least one more file to go through.
                  // Replace the EOF token with a formfeed.
                  *tok =
                    make_token_t (NEW_STRING (string_t_CP ()),
                                  NEW_STRING (string_t_formfeed ()),
                                  (*tok)->loc);

                  open_one_of_multiple_files (g);
                }
            }
        }
    }
}

static token_getter_t
make_multiple_files_getter_t (size_t n, const char *filenames[n])
{
  _multiple_files_getter_t g = XMALLOC (struct _multiple_files_getter);
  g->get_token = &get_token_from_multiple_files_getter;
  g->filenames = filenames;
  g->n = n;
  g->i = -1;
  g->f = NULL;
  g->getter = NULL;
  return (token_getter_t) g;
}

VISIBLE buffered_token_getter_t
make_buffered_token_getter_from_source_files (size_t n,
                                              const char *filenames[n])
{
  token_getter_t g = make_multiple_files_getter_t (n, filenames);
  return make_buffered_token_getter_t (g);
}

VISIBLE buffered_token_getter_t
make_buffered_token_getter_from_serialized_tokens (const char *filename,
                                                   FILE *f)
{
  token_getter_t g =
    make_token_getter_from_serialized_tokens_t (filename, f);
  return make_buffered_token_getter_t (g);
}

struct token_putter_to_stream
{
  void (*put_token) (token_putter_t this_struct,
                     token_t tok, const char **error_message);

  const char *filename;
  FILE *f;

  // Serialize or do other output processing.
  void (*outputter) (const token_t tok, FILE *f,
                     const char **error_message);
};
typedef struct token_putter_to_stream *token_putter_to_stream_t;

static void
put_token_to_stream (token_putter_t this_struct,
                     token_t tok, const char **error_message)
{
  token_putter_to_stream_t p = (token_putter_to_stream_t) this_struct;
  *error_message = NULL;
  p->outputter (tok, p->f, error_message);
}

static void
serializing_outputter (const token_t tok, FILE *f,
                       const char **error_message)
{
  *error_message = NULL;
  serialize_token_t (tok, f);
}

VISIBLE token_putter_t
make_token_putter_to_stream_serialized_t (const char *filename, FILE *f)
{
  token_putter_to_stream_t p = XMALLOC (struct token_putter_to_stream);
  p->put_token = put_token_to_stream;
  p->filename = filename;
  p->f = f;
  p->outputter = serializing_outputter;
  return (token_putter_t) p;
}

struct _getter_with_mismatch_check
{
  void (*get_token) (buffered_token_getter_t this_struct,
                     token_t *tok, const char **error_message);
  void (*look_at_token) (buffered_token_getter_t this_struct, size_t,
                         token_t *tok, const char **error_message);

  buffered_token_getter_t getter;
  gl_list_t queue;
  bool mismatch_detected;
};
typedef struct _getter_with_mismatch_check
  *_getter_with_mismatch_check_t;

static void
get_for_mismatch_check (buffered_token_getter_t this_struct,
                        token_t *tok, const char **error_message)
{
  _getter_with_mismatch_check_t g =
    (_getter_with_mismatch_check_t) this_struct;
  g->getter->get_token (g->getter, tok, error_message);
  if (!g->mismatch_detected)
    gl_list_add_last (g->queue, *tok);
}

static void
peek_for_mismatch_check (buffered_token_getter_t this_struct,
                         size_t i, token_t *tok,
                         const char **error_message)
{
  _getter_with_mismatch_check_t g =
    (_getter_with_mismatch_check_t) this_struct;
  g->getter->look_at_token (g->getter, i, tok, error_message);
}

static bool
mismatch_check (buffered_token_getter_t output_getter, token_t tok)
{
  _getter_with_mismatch_check_t g =
    (_getter_with_mismatch_check_t) output_getter;
  if (tok != NULL && !g->mismatch_detected)
    {
      g->mismatch_detected = (gl_list_size (g->queue) == 0);
      if (!g->mismatch_detected)
        {
          token_t t = (token_t) gl_list_get_first (g->queue);
          g->mismatch_detected =
            (string_t_cmp (t->token_kind, tok->token_kind) != 0
             || string_t_cmp (t->token_value, tok->token_value) != 0);
          gl_list_remove_first (g->queue);
        }
    }
  return g->mismatch_detected;
}

#define MAKE_TOKEN_GETTER__ make_token_getter_with_mismatch_check
VISIBLE void
MAKE_TOKEN_GETTER__ (buffered_token_getter_t input_getter,
                     buffered_token_getter_t *output_getter,
                     const bool (**check_for_mismatch)
                     (buffered_token_getter_t, token_t))
{
  _getter_with_mismatch_check_t p =
    XMALLOC (struct _getter_with_mismatch_check);

  p->getter = input_getter;
  p->queue =
    gl_list_create_empty (GL_AVLTREE_LIST, NULL, NULL, NULL, true);
  p->mismatch_detected = false;

  p->get_token = &get_for_mismatch_check;
  p->look_at_token = &peek_for_mismatch_check;

  *output_getter = (buffered_token_getter_t) p;
  *check_for_mismatch = &mismatch_check;
}

struct _putter_with_mismatch_check
{
  void (*put_token) (token_putter_t this_struct,
                     token_t tok, const char **error_message);

  token_putter_t input_putter;
  buffered_token_getter_t output_getter;
  const bool (*check_for_mismatch) (buffered_token_getter_t, token_t);
};
typedef struct _putter_with_mismatch_check
  *_putter_with_mismatch_check_t;

static void
put_with_mismatch_check (token_putter_t this_struct,
                         token_t tok, const char **error_message)
{
  _putter_with_mismatch_check_t p =
    (_putter_with_mismatch_check_t) this_struct;
  (void) p->check_for_mismatch (p->output_getter, tok);
  p->input_putter->put_token (p->input_putter, tok, error_message);
}

VISIBLE token_putter_t
make_token_putter_with_mismatch_check (token_putter_t input_putter,
                                       buffered_token_getter_t
                                       output_getter,
                                       bool (*check_for_mismatch)
                                       (buffered_token_getter_t,
                                        token_t))
{
  _putter_with_mismatch_check_t p =
    XMALLOC (struct _putter_with_mismatch_check);
  p->put_token = &put_with_mismatch_check;
  p->input_putter = input_putter;
  p->output_getter = output_getter;
  p->check_for_mismatch = check_for_mismatch;
  return (token_putter_t) p;
}

VISIBLE void
print_token_t (const token_t tok, FILE *f)
{
  fprintf (f, "%s:%zu:%zu %s |%s|\n", tok->loc->filename,
           tok->loc->line_no, tok->loc->code_point_no,
           make_str_nul (tok->token_kind),
           make_str_nul (tok->token_value));
}

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
