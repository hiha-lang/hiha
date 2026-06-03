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
#include <xstrerror.h>
#include <libhiha/xalloc.h>
#include <libhiha/token_t.h>
#include <libhiha/workspaces.h>
#include <libhiha/string_t.h>
#include <libhiha/str_nul.h>
#include <libhiha/indexed_deque.h>
#include <libhiha/persistent_integer_trie.h>
#include <libhiha/indexed_data_file.h>

#define _(msgid) HIHA_GETTEXT (msgid)

struct token_getter_from_string
{
  /* This struct must be castable to a struct token_getter. */

  void (*get_token) (token_getter_t this_struct,
                     token_t *tok, const char **error_message);

  const char *filename;         /* Will equal NULL. */
  string_t str;
  size_t i;                     /* Index into str. */
  size_t line_no;               /* Starting at one, increasing with '\n'. */
  size_t i_code_point;          /* Zero-based, per line. */
  bool eof_reached;
};
typedef struct token_getter_from_string *token_getter_from_string_t;
static void get_token_from_string (token_getter_t, token_t *,
                                   const char **);

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
  tok->extension = NULL;
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

static void
get_token_from_string (token_getter_t getter, token_t *tok,
                       const char **error_message)
{
  token_getter_from_string_t g = (token_getter_from_string_t) getter;

  *error_message = NULL;

  struct text_location *loc = XMALLOC (struct text_location);
  loc->filename = g->filename;
  loc->line_no = g->line_no;
  loc->code_point_no = g->i_code_point + 1;

  struct string *tokval;
  g->eof_reached = (g->eof_reached || g->i == g->str->n);
  if (!g->eof_reached)
    {
      tokval = XMALLOC (struct string);
      tokval->n = 1;
      tokval->s = XNMALLOC_ATOMIC (1, uint32_t);
      tokval->s[0] = g->str->s[g->i];
      g->i += 1;
      if (tokval->s[0] == '\n')
        {
          g->line_no += 1;
          g->i_code_point = 0;
        }
      else
        g->i_code_point += 1;
    }

  *tok =
    (g->eof_reached)
    ? make_token_t_eof_eof (loc)
    : make_token_t (string_t_CP (), tokval, loc);
}

HIHA_VISIBLE token_getter_t
make_token_getter_from_string (string_t str)
{
  token_getter_from_string_t getter =
    XMALLOC (struct token_getter_from_string);

  getter->get_token = &get_token_from_string;

  getter->filename = NULL;
  getter->str = str;
  getter->i = 0;
  getter->line_no = 1;
  getter->i_code_point = 0;
  getter->eof_reached = (getter->str->n == 0);

  return (token_getter_t) getter;
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
  getter->buf = XNMALLOC_ATOMIC (getter->nbuf, char);
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
      str->s = XNMALLOC_ATOMIC (1, uint32_t);
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
  size_t *box = XMALLOC_ATOMIC (size_t);
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
  size_t *box = XMALLOC_ATOMIC (size_t);
  *box = i;
  return string_t_map_insert_only (map, str, box);
}

static size_t
string_t_retrieve_index (string_t_map_t map, string_t str)
{
  const void *p = string_t_map_search (map, str);
  return (p != NULL) ? *((const size_t *) p) : ((size_t) -1);
}

struct _buffered_token_getter
{
  void (*get_token) (buffered_token_getter_t this_struct,
                     token_t *tok, const char **error_message);
  void (*look_at_token) (buffered_token_getter_t this_struct, size_t,
                         token_t *tok, const char **error_message);
  void (*look_at_and_get_token) (buffered_token_getter_t this_struct,
                                 size_t, bool (*)(token_t),
                                 token_t *tok,
                                 const char **error_message);
  void (*push_back_token) (buffered_token_getter_t this_struct,
                           token_t tok, const char **error_message);
  void (*push_back_string) (buffered_token_getter_t this_struct,
                            string_t str, text_location_t loc,
                            const char **error_message);
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

static void
look_at_and_get_from_buffered_token (buffered_token_getter_t getter,
                                     size_t i, bool (*pred) (token_t),
                                     token_t *tok,
                                     const char **error_message)
{
  getter->look_at_token (getter, i, tok, error_message);
  if (*error_message == NULL && pred (*tok))
    {
      size_t j = 0;
      while (*error_message == NULL && j != i + 1)
        {
          token_t t;
          getter->get_token (getter, &t, error_message);
          j += 1;
        }
    }
}

static void
push_back_token_into_buffered_getter (buffered_token_getter_t getter,
                                      token_t tok,
                                      const char **error_message)
{
  *error_message = NULL;
  _buffered_token_getter_t g = (_buffered_token_getter_t) getter;
  g->buffer = indexed_deque_put_before_first (g->buffer, tok);
}

static void
push_back_string_into_buffered_getter (buffered_token_getter_t getter,
                                       string_t str,
                                       text_location_t loc,
                                       const char **error_message)
{
  *error_message = NULL;
  _buffered_token_getter_t g = (_buffered_token_getter_t) getter;
  for (size_t i = 0; i != str->n; i += 1)
    {
      struct string *cstr = XMALLOC (struct string);
      cstr->n = 1;
      cstr->s = XNMALLOC_ATOMIC (1, uint32_t);
      cstr->s[0] = str->s[str->n - 1 - i];
      token_t tok = make_token_t (string_t_CP (), cstr, loc);
      getter->push_back_token (getter, tok, error_message);
    }
}

HIHA_VISIBLE buffered_token_getter_t
make_buffered_token_getter_t (token_getter_t unbuffered_getter)
{
  _buffered_token_getter_t g = XMALLOC (struct _buffered_token_getter);
  g->getter = unbuffered_getter;
  g->buffer = NULL;
  g->get_token = &get_token_from_buffered_getter;
  g->look_at_token = &look_at_buffered_token;
  g->look_at_and_get_token = &look_at_and_get_from_buffered_token;
  g->push_back_token = &push_back_token_into_buffered_getter;
  g->push_back_string = &push_back_string_into_buffered_getter;
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
  // to "FSEP" token with a form feed value.
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
                  // Replace the EOF token with a form feed.
                  *tok =
                    make_token_t ((make_string_t ("FSEP")),
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

HIHA_VISIBLE int
token_t_cmp (token_t tok1, token_t tok2)
{
  int result = string_t_cmp (tok1->token_kind, tok2->token_kind);
  if (result == 0)
    result = string_t_cmp (tok1->token_value, tok2->token_value);
  return result;
}

/**********************************************************************/

static void
set_token_files_names (const char *filename_root,
                       const char **fn_root,
                       const char **path_root, const char **fn_tokens)
{
  *fn_root = xstrdup (filename_root);

  size_t nwdir = strlen (work_directory ());
  size_t nroot = strlen (filename_root);
  const char *toks = ".tokens";
  size_t ntoks = strlen (toks);
  size_t n = nwdir + 1 + nroot + ntoks + 1;
  char *fn = XCALLOC_ATOMIC (n, char);

  memcpy (fn, work_directory (), nwdir * sizeof (char));
  fn[nwdir] = '/';
  memcpy (fn + nwdir + 1, filename_root, nroot * sizeof (char));
  *path_root = xstrdup (fn);

  memcpy (fn + nwdir + 1 + nroot, toks, ntoks * sizeof (char));
  *fn_tokens = fn;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

HIHA_INT_TRIE_NODES_DECL (_df_str_nul_map_node, uint64_t, const char *);
HIHA_INT_TRIE_INSERT_DEFN (_df_str_nul_map_insert,
                           _df_str_nul_map_node, uint64_t,
                           const char *);
HIHA_INT_TRIE_SEARCH_DEFN (_df_str_nul_map_search, _df_str_nul_map_node,
                           uint64_t);

HIHA_INT_TRIE_NODES_DECL (_df_string_t_map_node, uint64_t, string_t);
HIHA_INT_TRIE_INSERT_DEFN (_df_string_t_map_insert,
                           _df_string_t_map_node, uint64_t, string_t);
HIHA_INT_TRIE_SEARCH_DEFN (_df_string_t_map_search,
                           _df_string_t_map_node, uint64_t);

struct token_getter_from_files
{
  void (*get_token) (token_getter_t this_struct,
                     token_t *tok, const char **error_message);

  const char *filename_root;
  const char *path_root;
  const char *filename_tokens;
  FILE *f_tokens;
  indexed_data_file_t df;       /* Filenames, strings, extension fields. */
  _df_str_nul_map_node_t filenames;
  _df_string_t_map_node_t strings;
  int64_t n_tokens;
  int64_t i_token;
};

static void
str_nul_from_data_file (indexed_data_file_t df, int64_t index,
                        _df_str_nul_map_node_t *map,
                        const char **s, const char **error_message)
{
  if (*error_message == NULL)
    if (index < 0)
      *s = NULL;
    else
      {
        _df_str_nul_map_node_leaf_t leaf =
          _df_str_nul_map_search (*map, index);
        if (leaf != NULL)
          *s = leaf->value;
        else
          {
            void *data;
            size_t n_data;
            read_from_indexed_data_file (df, index, &data, &n_data);
            char *p = data;
            p[n_data - 1] = '\0';       /* Guarantee a terminating NUL. */
            *s = p;
            *map = _df_str_nul_map_insert (*map, index, *s);
          }
      }
}

static void
string_t_from_data_file (indexed_data_file_t df, int64_t index,
                         _df_string_t_map_node_t *map,
                         string_t *str, const char **error_message)
{
  if (*error_message == NULL)
    if (index < 0)
      *str = NULL;
    else
      {
        _df_string_t_map_node_leaf_t leaf =
          _df_string_t_map_search (*map, index);
        if (leaf != NULL)
          *str = leaf->value;
        else
          {
            void *data;
            size_t n_data;
            read_from_indexed_data_file (df, index, &data, &n_data);
            char *p = data;

            size_t n_u32;
            uint32_t *u32 = u8_to_u32 (data, n_data, NULL, &n_u32);
            int err_number = errno;
            if (u32 == NULL)
              *error_message = xstrerror (NULL, err_number);
            else
              {
                struct string *st = XMALLOC (struct string);
                st->s = u32;
                st->n = n_u32;
                *str = st;
                *map = _df_string_t_map_insert (*map, index, *str);
              }
          }
      }
}

static int64_t
count_tokens_in_file (token_getter_from_files_t p)
{
  int retval = fseek (p->f_tokens, 0, SEEK_END);
  int err_number = errno;
  if (retval != 0)
    {
      error (exit_failure, err_number, "%s", p->filename_tokens);
      abort ();
    }
  long offset = ftell (p->f_tokens);
  err_number = errno;
  if (offset == -1)
    {
      error (exit_failure, err_number, "%s", p->filename_tokens);
      abort ();
    }
  return offset / (6 * sizeof (int64_t));
}

static void
read_token_entry (int64_t buf[6], token_getter_from_files_t p)
{
  int retval =
    fseek (p->f_tokens, p->i_token * 6 * sizeof (int64_t), SEEK_SET);
  int err_number = errno;
  if (retval != 0)
    {
      error (exit_failure, err_number, "%s", p->filename_tokens);
      abort ();
    }
  int num_read = fread (buf, sizeof (int64_t), 6, p->f_tokens);
  if (num_read != 6)
    {
      error (exit_failure, err_number, "%s", p->filename_tokens);
      abort ();
    }
  if (p->i_token != p->n_tokens - 1)
    p->i_token += 1;
}

static void
get_token_from_files (token_getter_t this_struct, token_t *tok,
                      const char **error_message)
{
  token_getter_from_files_t p = (token_getter_from_files_t) this_struct;

  *error_message = NULL;

  int64_t buf[6];

  read_token_entry (buf, p);

  const int64_t i_filename = buf[0];
  const int64_t line_no = buf[1];
  const int64_t code_point_no = buf[2];
  const int64_t i_token_kind = buf[3];
  const int64_t i_token_value = buf[4];
  const int64_t i_extension = buf[5];

  struct text_location *loc = XMALLOC (struct text_location);
  str_nul_from_data_file (p->df, i_filename, &p->filenames,
                          &loc->filename, error_message);
  loc->line_no = line_no;
  loc->code_point_no = code_point_no;

  string_t token_kind;
  string_t_from_data_file (p->df, i_token_kind, &p->strings,
                           &token_kind, error_message);

  string_t token_value;
  string_t_from_data_file (p->df, i_token_value, &p->strings,
                           &token_value, error_message);

  if (*error_message == NULL && 0 <= i_extension)
    *error_message =
      _("token_t extension fields are not yet supported");

  if (*error_message == NULL)
    *tok = make_token_t (token_kind, token_value, loc);
}

HIHA_VISIBLE token_getter_from_files_t
make_token_getter_from_files_t (const char *filename_root)
{
  token_getter_from_files_t p =
    XMALLOC (struct token_getter_from_files);

  set_token_files_names (filename_root, &p->filename_root,
                         &p->path_root, &p->filename_tokens);

  FILE *f = fopen (p->filename_tokens, "rb");
  int err_number = errno;
  if (f == NULL)
    {
      error (exit_failure, err_number, "%s", p->filename_tokens);
      abort ();
    }

  p->get_token = &get_token_from_files;
  p->f_tokens = f;
  p->df = open_indexed_data_file (p->path_root);
  p->filenames = NULL;
  p->strings = NULL;
  p->n_tokens = count_tokens_in_file (p);
  p->i_token = 0;
  return p;
}

static void
close_getter_f_tokens (token_getter_from_files_t p)
{
  if (p->f_tokens != NULL)
    {
      int retval = fclose (p->f_tokens);
      int err_number = errno;
      if (retval != 0)
        {
          error (exit_failure, err_number, "%s", p->filename_tokens);
          abort ();
        }
      p->f_tokens = NULL;
    }
}

HIHA_VISIBLE void
close_token_getter_files (token_getter_from_files_t p)
{
  close_indexed_data_file (p->df);
  close_getter_f_tokens (p);
}

HIHA_VISIBLE void
remove_token_getter_files (token_getter_from_files_t p)
{
  remove_indexed_data_file (p->df);
  close_getter_f_tokens (p);
  remove (p->filename_tokens);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

struct token_putter_to_files
{
  void (*put_token) (token_putter_t this_struct,
                     token_t tok, const char **error_message);

  const char *filename_root;
  const char *path_root;
  const char *filename_tokens;
  FILE *f_tokens;
  indexed_data_file_t df;       /* Filenames, strings, extension fields. */
  str_nul_map_t filenames;      /* Indices of filenames. */
  string_t_map_t strings;       /* Indices of strings. */
};

static void
str_nul_in_data_file (indexed_data_file_t df, str_nul_map_t *map,
                      const char *s, int64_t *index,
                      const char **error_message)
{
  if (*error_message == NULL)
    {
      *index = -1;
      if (s != NULL)
        {
          const int64_t *p_index = str_nul_map_search (*map, s);
          if (p_index != NULL)
            *index = *p_index;
          else
            {
              size_t *i_sz = XMALLOC_ATOMIC (size_t);
              write_to_indexed_data_file
                (df, s, (strlen (s) + 1) * sizeof (char), i_sz);
              *index = *i_sz;
              *map = str_nul_map_insert_only (*map, s, i_sz);
            }
        }
    }
}

static void
string_t_in_data_file (indexed_data_file_t df, string_t_map_t *map,
                       string_t str, int64_t *index,
                       const char **error_message)
{
  /* To save space, write the string in UTF-8. */

  if (*error_message == NULL)
    {
      *index = -1;
      if (str != NULL)
        {
          const int64_t *p_index = string_t_map_search (*map, str);
          if (p_index != NULL)
            *index = *p_index;
          else
            {
              size_t n_u8;
              uint8_t *u8 = u32_to_u8 (str->s, str->n, NULL, &n_u8);
              int err_number = errno;
              if (u8 == NULL)
                *error_message = xstrerror (NULL, err_number);
              else
                {
                  size_t *i_sz = XMALLOC_ATOMIC (size_t);
                  write_to_indexed_data_file
                    (df, u8, n_u8 * sizeof (uint8_t), i_sz);
                  *index = *i_sz;
                  *map = string_t_map_insert_only (*map, str, i_sz);
                }
            }
        }
    }
}

static void
put_token_to_files (token_putter_t this_struct, token_t tok,
                    const char **error_message)
{
  token_putter_to_files_t p = (token_putter_to_files_t) this_struct;
  *error_message = NULL;

  int64_t i_filename;
  int64_t i_token_kind;
  int64_t i_token_value;
  int64_t i_extension;

  str_nul_in_data_file (p->df, &p->filenames, tok->loc->filename,
                        &i_filename, error_message);
  string_t_in_data_file (p->df, &p->strings, tok->token_kind,
                         &i_token_kind, error_message);
  string_t_in_data_file (p->df, &p->strings, tok->token_value,
                         &i_token_value, error_message);

  /* Only NULL extensions are supported, currently. */
  assert (tok->extension == NULL);
  i_extension = -1;

  if (*error_message == NULL)
    {
      int64_t buf[6];
      buf[0] = i_filename;
      buf[1] = tok->loc->line_no;
      buf[2] = tok->loc->code_point_no;
      buf[3] = i_token_kind;
      buf[4] = i_token_value;
      buf[5] = i_extension;
      int num_written = fwrite (buf, sizeof (int64_t), 6, p->f_tokens);
      int err_number = errno;
      if (num_written != 6)
        *error_message = xstrerror (NULL, err_number);
    }
}

HIHA_VISIBLE token_putter_to_files_t
make_token_putter_to_files_t (const char *filename_root)
{
  token_putter_to_files_t p = XMALLOC (struct token_putter_to_files);

  set_token_files_names (filename_root, &p->filename_root,
                         &p->path_root, &p->filename_tokens);

  FILE *f = fopen (p->filename_tokens, "wb");
  int err_number = errno;
  if (f == NULL)
    {
      error (exit_failure, err_number, "%s", p->filename_tokens);
      abort ();
    }

  p->put_token = &put_token_to_files;
  p->f_tokens = f;
  p->df = create_indexed_data_file (p->path_root);
  p->filenames = NULL;
  p->strings = NULL;
  return p;
}

static void
close_putter_f_tokens (token_putter_to_files_t p)
{
  if (p->f_tokens != NULL)
    {
      int retval = fclose (p->f_tokens);
      int err_number = errno;
      if (retval != 0)
        {
          error (exit_failure, err_number, "%s", p->filename_tokens);
          abort ();
        }
      p->f_tokens = NULL;
    }
}

HIHA_VISIBLE void
close_token_putter_files (token_putter_to_files_t p)
{
  close_indexed_data_file (p->df);
  close_putter_f_tokens (p);
}

HIHA_VISIBLE void
remove_token_putter_files (token_putter_to_files_t p)
{
  remove_indexed_data_file (p->df);
  close_putter_f_tokens (p);
  remove (p->filename_tokens);
}

/**********************************************************************/

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
