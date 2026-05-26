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

#ifndef __LIBHAHA__TOKEN_T_H__INCLUDED__
#define __LIBHAHA__TOKEN_T_H__INCLUDED__

#include <stdio.h>
#include <libhiha/string_t.h>

/*--------------------------------------------------------------------*/

struct token
{
  string_t token_kind;
  string_t token_value;
  text_location_t loc;
};
typedef const struct token *token_t;

struct serialized_strings;
typedef struct serialized_strings *serialized_strings_t;

token_t make_token_t (string_t token_kind, string_t token_value,
                      text_location_t loc);
token_t make_token_t_eof_eof (text_location_t loc);
bool token_t_is_eof_eof (token_t tok);
void serialize_token_t (const token_t tok, serialized_strings_t strings,
                        FILE *f);
void print_token_t (const token_t tok, FILE *f);

struct token_getter;
typedef struct token_getter *token_getter_t;
struct token_getter
{
  void (*get_token) (token_getter_t this_struct,
                     token_t *tok, const char **error_message);
};

token_getter_t make_token_getter_from_string (string_t);

token_getter_t make_token_getter_from_source_file_t
  (const char *filename, FILE *f);

token_getter_t make_token_getter_from_serialized_tokens_t
  (const char *filename, FILE *f);

struct buffered_token_getter;
typedef struct buffered_token_getter *buffered_token_getter_t;
struct buffered_token_getter
{
  void (*get_token) (buffered_token_getter_t this_struct,
                     token_t *tok, const char **error_message);

  void (*look_at_token) (buffered_token_getter_t this_struct, size_t,
                         token_t *tok, const char **error_message);

  void (*push_back_token) (buffered_token_getter_t this_struct,
                           token_t tok);

  /* Push back the code points of a string as "CP" tokens. */
  void (*push_back_string) (buffered_token_getter_t this_struct,
                            string_t str, text_location_t loc);
};

buffered_token_getter_t make_buffered_token_getter_t
  (token_getter_t unbuffered_getter);

/* Play the files in sequence, converting all the EOF but the last to
   formfeed. */
buffered_token_getter_t make_buffered_token_getter_from_source_files
  (size_t n, const char *filenames[n]);

buffered_token_getter_t
  make_buffered_token_getter_from_serialized_tokens
  (const char *filename, FILE *f);

/* The first argument to check_for_mismatch MUST be the value that was
   returned as *output_getter. The second argument should be either a
   token outputted by the Pratt parser, to include it in the check
   whether there are any mismatches between inputs and outputs, or
   NULL, to check whether a mismatch has been detected. Thus we can
   achieve fixed point detection in lexical analysis. */
void make_token_getter_with_mismatch_check
  (buffered_token_getter_t input_getter,
   buffered_token_getter_t *output_getter,
   const bool (**check_for_mismatch) (buffered_token_getter_t,
                                      token_t));

struct token_putter;
typedef struct token_putter *token_putter_t;
struct token_putter
{
  void (*put_token) (token_putter_t this_struct,
                     token_t tok, const char **error_message);
};

token_putter_t make_token_putter_to_stream_serialized_t
  (const char *filename, FILE *f);

token_putter_t make_token_putter_with_mismatch_check
  (token_putter_t input_putter, buffered_token_getter_t output_getter,
   bool (*check_for_mismatch) (buffered_token_getter_t, token_t));

/*--------------------------------------------------------------------*/
/* string_t_cmp first by token_kind, then by token_value. */

int token_t_cmp (token_t tok1, token_t tok2);

/*--------------------------------------------------------------------*/

struct token_t_keyval
{
  token_t key;
  const void *value;
};
typedef const struct token_t_keyval *token_t_keyval_t;

DECLARE_HIHA_PERSISTENT_VECTOR_DATATYPE (, token_t_vector, token_t, 5);
DECLARE_HIHA_PERSISTENT_VECTOR_DATATYPE (, token_t_keyval_vector,
                                         token_t_keyval_t, 5);

/*--------------------------------------------------------------------*/

struct token_t_hash_context;
typedef struct token_t_hash_context *token_t_hash_context_t;

token_t_hash_context_t token_t_hash_init (token_t str);
uint64_t token_t_hash (token_t_hash_context_t context, unsigned int i);

/*--------------------------------------------------------------------*/
/* Persistent unordered maps. */

struct token_t_map;
typedef const struct token_t_map *token_t_map_t;

size_t token_t_map_size (token_t_map_t map);

const void *token_t_map_search (token_t_map_t map, token_t key);

token_t_map_t token_t_map_insert_or_replace (token_t_map_t map,
                                             token_t key,
                                             const void *value);
token_t_map_t token_t_map_insert_only (token_t_map_t map,
                                       token_t key, const void *value);
token_t_map_t token_t_map_replace_only (token_t_map_t map,
                                        token_t key, const void *value);

token_t_map_t token_t_map_delete (token_t_map_t map, token_t key);

token_t_vector_t token_t_map_keys (token_t_map_t map);
voidp_vector_t token_t_map_values (token_t_map_t map);
token_t_keyval_vector_t token_t_map_associations (token_t_map_t map);

/*--------------------------------------------------------------------*/
/* Persistent ordered maps. */

struct token_t_omap;
typedef const struct token_t_omap *token_t_omap_t;

size_t token_t_omap_size (token_t_omap_t omap);

const void *token_t_omap_search (token_t_omap_t omap, token_t key);

/* token_t_omap_init creates a new empty map with a custom order.
   (The default order, which is token_t_cmp applied to the keys, is
   obtained by using NULL as the empty map.) */
token_t_omap_t token_t_omap_init (int (*compare) (token_t_keyval_t,
                                                  token_t_keyval_t));

token_t_omap_t token_t_omap_insert_or_replace (token_t_omap_t omap,
                                               token_t key,
                                               const void *value);
token_t_omap_t token_t_omap_insert_only (token_t_omap_t omap,
                                         token_t key,
                                         const void *value);
token_t_omap_t token_t_omap_replace_only (token_t_omap_t omap,
                                          token_t key,
                                          const void *value);

token_t_omap_t token_t_omap_delete (token_t_omap_t omap, token_t key);

/* direction = 1 forwards, -1 backwards */
token_t_vector_t token_t_omap_keys (token_t_omap_t omap, int direction);
voidp_vector_t token_t_omap_values (token_t_omap_t omap, int direction);
token_t_keyval_vector_t token_t_omap_associations (token_t_omap_t omap,
                                                   int direction);

/*--------------------------------------------------------------------*/

#endif /* __LIBHAHA__TOKEN_T_H__INCLUDED__ */

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
