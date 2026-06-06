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
  void *extension;              /* NULL for simple lexical tokens. */
};
typedef const struct token *token_t;

token_t make_token_t (string_t token_kind, string_t token_value,
                      text_location_t loc);
token_t make_token_t_eof_eof (text_location_t loc);
bool token_t_is_eof_eof (token_t tok);

struct token_getter;
typedef struct token_getter *token_getter_t;
struct token_getter
{
  void (*get_token) (token_getter_t this_struct,
                     token_t *tok, const char **error_message);
};

#define to_token_getter_t(p) ((token_getter_t) (p))

token_getter_t make_token_getter_from_string (string_t);

token_getter_t make_token_getter_from_source_file_t
  (const char *filename, FILE *f);

struct buffered_token_getter;
typedef struct buffered_token_getter *buffered_token_getter_t;
struct buffered_token_getter
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

  /* Push back the code points of a string as "CP" tokens. */
  void (*push_back_string) (buffered_token_getter_t this_struct,
                            string_t str, text_location_t loc,
                            const char **error_message);
};

buffered_token_getter_t make_buffered_token_getter_t
  (token_getter_t unbuffered_getter);

/* Play the files in sequence, converting all the EOF but the last to
   formfeed. */
buffered_token_getter_t make_buffered_token_getter_from_source_files
  (size_t n, const char *filenames[n]);

struct token_putter;
typedef struct token_putter *token_putter_t;
struct token_putter
{
  void (*put_token) (token_putter_t this_struct,
                     token_t tok, const char **error_message);
};

#define to_token_putter_t(p) ((token_putter_t) (p))

/*--------------------------------------------------------------------*/

struct token_getter_from_files;
typedef struct token_getter_from_files *token_getter_from_files_t;

token_getter_from_files_t make_token_getter_from_files_t (const char
                                                          *filename_root);
void close_token_getter_files (token_getter_from_files_t);
void remove_token_getter_files (token_getter_from_files_t);

struct token_putter_to_files;
typedef struct token_putter_to_files *token_putter_to_files_t;

token_putter_to_files_t make_token_putter_to_files_t (const char
                                                      *filename_root);
void close_token_putter_files (token_putter_to_files_t);
void remove_token_putter_files (token_putter_to_files_t);

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

token_t_hash_context_t token_t_hash_init (token_t tok);
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
/* Persistent unordered sets. */

struct token_t_set;
typedef const struct token_t_set *token_t_set_t;

size_t token_t_set_size (token_t_set_t set);

bool token_t_set_contains (token_t_set_t set, token_t element);
token_t_set_t token_t_set_adjoin (token_t_set_t set, token_t element);
token_t_set_t token_t_set_delete (token_t_set_t set, token_t element);

extern token_t_set_t const token_t_set_end;     /* Sentinel for “...” */

token_t_set_t token_t_set_union (...);
token_t_set_t token_t_set_intersection (...);
token_t_set_t token_t_set_difference (token_t_set_t set, ...);
token_t_set_t token_t_set_symmetric_difference (token_t_set_t set1,
                                                token_t_set_t set2);

bool token_t_set_equal (token_t_set_t set, ...);
bool token_t_set_subset (token_t_set_t set, ...);
bool token_t_set_proper_subset (token_t_set_t set, ...);
bool token_t_set_superset (token_t_set_t set, ...);
bool token_t_set_proper_superset (token_t_set_t set, ...);

token_t_vector_t token_t_set_elements (token_t_set_t set);

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
