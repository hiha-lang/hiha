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
#include <libhiha/token_t.h>
#include <libhiha/str_nul.h>
#include <libhiha/indexed_deque.h>

#define _(msgid) HIHA_GETTEXT (msgid)

struct serialized_strings
{
  size_t filenames_index;
  size_t token_kinds_index;
  size_t token_values_index;

  str_nul_map_t filename_to_index;
  string_t_map_t token_kind_to_index;
  string_t_map_t token_value_to_index;
};
typedef struct serialized_strings *serialized_strings_t;

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

struct deserialized_strings
{
  voidp_vector_t filenames;
  voidp_vector_t token_kinds;
  voidp_vector_t token_values;
};
typedef struct deserialized_strings *deserialized_strings_t;

static deserialized_strings_t
make_deserialized_strings_t (void)
{
  deserialized_strings_t result = XMALLOC (struct deserialized_strings);
  result->filenames = NULL;
  result->token_kinds = NULL;
  result->token_values = NULL;
  return result;
}

struct token_getter_from_serialized_tokens
{
  /* This struct must be castable to a struct token_getter. */

  void (*get_token) (token_getter_t this_struct,
                     token_t *tok, const char **error_message);

  const char *filename;
  FILE *f;
  char *buf;
  size_t nbuf;                  /* The size of the buf. */
  deserialized_strings_t strings;
  bool eof_reached;
};
typedef struct token_getter_from_serialized_tokens
  *token_getter_from_serialized_tokens_t;
static void get_token_from_serialized_tokens (token_getter_t, token_t *,
                                              const char **);

HIHA_VISIBLE token_t
make_token_t (string_t token_kind, string_t token_value,
              text_location_t loc)
{
  struct token *tok = XMALLOC (struct token);
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
  struct text_location *loc = XMALLOC (struct text_location);
  loc->filename = filename;
  loc->line_no = line_no;
  loc->code_point_no = code_point_no;
  return make_token_t (token_kind, token_value, loc);
}

static token_t
_make_token_t_eof_eof (const char *filename, size_t line_no,
                       size_t code_point_no)
{
  struct text_location *loc = XMALLOC (struct text_location);
  loc->filename = filename;
  loc->line_no = line_no;
  loc->code_point_no = code_point_no;
  return make_token_t_eof_eof (loc);
}

HIHA_VISIBLE token_t
make_token_t_eof_eof (text_location_t loc)
{
  return make_token_t (string_t_EOF (), string_t_EOF (), loc);
}

HIHA_VISIBLE bool
token_t_is_eof_eof (token_t tok)
{
  return (0 == string_t_cmp (string_t_EOF (), tok->token_kind)
          && 0 == string_t_cmp (string_t_EOF (), tok->token_value));
}

HIHA_VISIBLE token_getter_t
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
          struct text_location *loc = XMALLOC (struct text_location);
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
      struct string *str = XMALLOC (struct string);
      str->s = XNMALLOC (1, uint32_t);
      str->s[0] = g->sbuf->s[g->i_code_point];
      str->n = 1;
      *tok =
        _make_token_t (string_t_CP (), str, g->filename,
                       g->line_no, g->i_code_point + 1);
      g->i_code_point += 1;
    }
  else if (was_end_of_line)
    *tok = _make_token_t_eof_eof (g->filename, g->line_no + 1, 0);
  else
    *tok =
      _make_token_t_eof_eof (g->filename, g->line_no,
                             g->i_code_point + 1);
}

static str_nul_map_t
str_nul_insert_index (str_nul_map_t map, const char *str, size_t i)
{
  size_t *box = XMALLOC (size_t);
  *box = i;
  return str_nul_map_insert_only (map, str, box);
}

static size_t
str_nul_retrieve_index (str_nul_map_t map, const char *str)
{
  const void *p = str_nul_map_search (map, str);
  return (p != NULL) ? *((const size_t *) p) : ((size_t) -1);
}

static string_t_map_t
string_t_insert_index (string_t_map_t map, string_t str, size_t i)
{
  size_t *box = XMALLOC (size_t);
  *box = i;
  return string_t_map_insert_only (map, str, box);
}

static size_t
string_t_retrieve_index (string_t_map_t map, string_t str)
{
  const void *p = string_t_map_search (map, str);
  return (p != NULL) ? *((const size_t *) p) : ((size_t) -1);
}

static serialized_strings_t
make_serialized_strings_t (void)
{
  serialized_strings_t p = XMALLOC (struct serialized_strings);

  p->filenames_index = 0;
  p->token_kinds_index = 0;
  p->token_values_index = 0;

  p->filename_to_index = NULL;
  p->token_kind_to_index = NULL;
  p->token_value_to_index = NULL;

  /* The NULL filename will be represented by the index 0. */
  p->filename_to_index =
    str_nul_insert_index (p->filename_to_index, NULL, 0);

  return p;
}

HIHA_VISIBLE void
serialize_token_t (const token_t tok, serialized_strings_t strings,
                   FILE *f)
{
  /* The serialization done here is NOT interchangeable between
     machines. */

  size_t i = str_nul_retrieve_index (strings->filename_to_index,
                                     tok->loc->filename);
  //  size_t i = filename_to_index (strings->filenames, tok->loc->filename);
  if (i == (size_t) (-1))
    {
      i = strings->filenames_index;
      strings->filenames_index += 1;
      strings->filename_to_index =
        str_nul_insert_index (strings->filename_to_index,
                              xstrdup (tok->loc->filename), i);
      //      strings->filenames =
      //        voidp_vector_push (strings->filenames, tok->loc->filename);
      //      i = voidp_vector_length (strings->filenames) - 1;

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

  size_t j = string_t_retrieve_index (strings->token_kind_to_index,
                                      tok->token_kind);
  if (j == (size_t) (-1))
    {
      j = strings->token_kinds_index;
      strings->token_kinds_index += 1;
      strings->token_kind_to_index =
        string_t_insert_index (strings->token_kind_to_index,
                               tok->token_kind, j);

      /* Write the token_kind in BASE64-encoding of the UTF32-encoded
         string, in native byte order, after a number that will be
         used in place of the string. */
      size_t inlen1 = tok->token_kind->n * sizeof (uint32_t);
      size_t outlen1 = BASE64_LENGTH (inlen1) + 1;
      char *buf1 = XNMALLOC (outlen1, char);
      base64_encode ((const char *) tok->token_kind->s, inlen1, buf1,
                     outlen1);
      fprintf (f, "K %zu %zu %s\n", j, tok->token_kind->n, buf1);
      free (buf1);
    }

  size_t k = string_t_retrieve_index (strings->token_value_to_index,
                                      tok->token_value);
  if (k == (size_t) (-1))
    {
      k = strings->token_values_index;
      strings->token_values_index += 1;
      strings->token_value_to_index =
        string_t_insert_index (strings->token_value_to_index,
                               tok->token_value, k);

      /* Write the token_value in BASE64-encoding of the UTF32-encoded
         string, in native byte order, after a number that will be
         used in place of the string. */
      size_t inlen2 = tok->token_value->n * sizeof (uint32_t);
      size_t outlen2 = BASE64_LENGTH (inlen2) + 1;
      char *buf2 = XNMALLOC (outlen2, char);
      base64_encode ((const char *) tok->token_value->s, inlen2, buf2,
                     outlen2);
      fprintf (f, "V %zu %zu %s\n", k, tok->token_value->n, buf2);
      free (buf2);
    }

  fprintf (f, "T %zu %zu %zu %zu %zu\n", i, tok->loc->line_no,
           tok->loc->code_point_no, j, k);
}

HIHA_VISIBLE token_getter_t
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

  getter->strings = make_deserialized_strings_t ();

  getter->eof_reached = false;

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
  if (retval != 4 || F != 'F'
      || index != voidp_vector_length (g->strings->filenames)
      || nb64 != strlen (b64))
    *nread = -101;
  else
    {
      char *s = XCALLOC (string_size + 1, char);
      idx_t outlen = string_size;
      base64_decode (b64, nb64, s, &outlen);
      free (b64);
      g->strings->filenames =
        voidp_vector_push (g->strings->filenames, s);
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
  if (retval != 4 || K != 'K'
      || index != voidp_vector_length (g->strings->token_kinds)
      || nb64 != strlen (b64))
    *nread = -102;
  else
    {
      struct string *s = XMALLOC (struct string);
      s->n = string_size;
      s->s = XNMALLOC (s->n, uint32_t);
      idx_t outlen = s->n * sizeof (uint32_t);
      base64_decode (b64, nb64, (char *) s->s, &outlen);
      free (b64);
      g->strings->token_kinds =
        voidp_vector_push (g->strings->token_kinds, s);
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
  if (retval != 4 || V != 'V'
      || index != voidp_vector_length (g->strings->token_values)
      || nb64 != strlen (b64))
    *nread = -103;
  else
    {
      struct string *s = XMALLOC (struct string);
      s->n = string_size;
      s->s = XNMALLOC (s->n, uint32_t);
      idx_t outlen = s->n * sizeof (uint32_t);
      base64_decode (b64, nb64, (char *) s->s, &outlen);
      free (b64);
      g->strings->token_values =
        voidp_vector_push (g->strings->token_values, s);
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
      || voidp_vector_length (g->strings->filenames) <= i_filename
      || voidp_vector_length (g->strings->token_kinds) <= i_token_kind
      || voidp_vector_length (g->strings->token_values) <=
      i_token_value)
    *nread = -104;
  else
    {
      const char *filename =
        voidp_vector_ref (g->strings->filenames, i_filename);
      string_t tokkind =
        voidp_vector_ref (g->strings->token_kinds, i_token_kind);
      string_t tokvalue =
        voidp_vector_ref (g->strings->token_values, i_token_value);
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

  if (g->eof_reached)
    *tok = make_token_t_eof_eof (NULL);
  else
    {
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
      if (*tok != NULL)
        g->eof_reached = token_t_is_eof_eof (*tok);
      else
        {
          char s[1000];
          snprintf (s, 1000, _("error involving serialized tokens"));
          *error_message = xstrdup (s);
        }
    }
}

struct _buffered_token_getter
{
  void (*get_token) (buffered_token_getter_t this_struct,
                     token_t *tok, const char **error_message);
  void (*look_at_token) (buffered_token_getter_t this_struct, size_t,
                         token_t *tok, const char **error_message);
  token_getter_t getter;
  indexed_deque_t buffer;
};
typedef struct _buffered_token_getter *_buffered_token_getter_t;

static void
get_token_from_buffered_getter (buffered_token_getter_t getter,
                                token_t *tok,
                                const char **error_message)
{
  _buffered_token_getter_t g = (_buffered_token_getter_t) getter;
  *error_message = NULL;

  if (indexed_deque_size (g->buffer) != 0)
    {
      *tok = (token_t) indexed_deque_get_first (g->buffer);
      g->buffer = indexed_deque_delete_first (g->buffer);
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

  while (*error_message == NULL && indexed_deque_size (g->buffer) <= i)
    {
      token_t t;
      g->getter->get_token (g->getter, &t, error_message);
      if (*error_message == NULL)
        g->buffer = indexed_deque_put_after_last (g->buffer, t);
    }
  if (*error_message == NULL)
    *tok = (token_t) indexed_deque_get (g->buffer, i);
}

HIHA_VISIBLE buffered_token_getter_t
make_buffered_token_getter_t (token_getter_t unbuffered_getter)
{
  _buffered_token_getter_t g = XMALLOC (struct _buffered_token_getter);
  g->getter = unbuffered_getter;
  g->buffer = NULL;
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
    *tok = make_token_t_eof_eof (NULL);
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
          if (token_t_is_eof_eof (*tok))
            {
              // Close the finished file.
              fclose (g->f);

              g->i += 1;
              if (g->i != g->n)
                {
                  // There is at least one more file to go through.
                  // Replace the EOF token with a formfeed.
                  *tok =
                    make_token_t ((string_t_CP ()),
                                  (string_t_formfeed ()), (*tok)->loc);

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

HIHA_VISIBLE buffered_token_getter_t
make_buffered_token_getter_from_source_files (size_t n,
                                              const char *filenames[n])
{
  token_getter_t g = make_multiple_files_getter_t (n, filenames);
  return make_buffered_token_getter_t (g);
}

HIHA_VISIBLE buffered_token_getter_t
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

  serialized_strings_t strings;

  // Serialize or do other output processing.
  void (*outputter) (const token_t tok, FILE *f,
                     serialized_strings_t strings,
                     const char **error_message);
};
typedef struct token_putter_to_stream *token_putter_to_stream_t;

static void
put_token_to_stream (token_putter_t this_struct,
                     token_t tok, const char **error_message)
{
  token_putter_to_stream_t p = (token_putter_to_stream_t) this_struct;
  *error_message = NULL;
  p->outputter (tok, p->f, p->strings, error_message);
}

static void
serializing_outputter (const token_t tok, FILE *f,
                       serialized_strings_t strings,
                       const char **error_message)
{
  *error_message = NULL;
  serialize_token_t (tok, strings, f);
}

HIHA_VISIBLE token_putter_t
make_token_putter_to_stream_serialized_t (const char *filename, FILE *f)
{
  token_putter_to_stream_t p = XMALLOC (struct token_putter_to_stream);
  p->put_token = put_token_to_stream;
  p->filename = filename;
  p->f = f;
  p->strings = make_serialized_strings_t ();
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
  indexed_deque_t queue;
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
    g->queue = indexed_deque_put_after_last (g->queue, *tok);
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
      g->mismatch_detected = (indexed_deque_size (g->queue) == 0);
      if (!g->mismatch_detected)
        {
          token_t t = (token_t) indexed_deque_get_first (g->queue);
          g->mismatch_detected =
            (string_t_cmp (t->token_kind, tok->token_kind) != 0
             || string_t_cmp (t->token_value, tok->token_value) != 0);
          g->queue = indexed_deque_delete_first (g->queue);
        }
    }
  return g->mismatch_detected;
}

#define MAKE_TOKEN_GETTER__ make_token_getter_with_mismatch_check
HIHA_VISIBLE void
MAKE_TOKEN_GETTER__ (buffered_token_getter_t input_getter,
                     buffered_token_getter_t *output_getter,
                     const bool (**check_for_mismatch)
                     (buffered_token_getter_t, token_t))
{
  _getter_with_mismatch_check_t p =
    XMALLOC (struct _getter_with_mismatch_check);

  p->getter = input_getter;
  p->queue = NULL;
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

HIHA_VISIBLE token_putter_t
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

HIHA_VISIBLE void
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
