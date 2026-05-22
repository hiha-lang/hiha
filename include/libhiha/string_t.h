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

#ifndef __LIBHAHA__STRING_T_H__INCLUDED__
#define __LIBHAHA__STRING_T_H__INCLUDED__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistr.h>
#include <unicase.h>
#include <uniconv.h>
#include <uninorm.h>
#include <libhiha/persistent_vector.h>

#define HIHA_PURE [[gnu::pure]]

/*--------------------------------------------------------------------*/

struct string
{
  uint32_t *s;
  size_t n;
};
typedef const struct string *string_t;

struct text_location
{
  const char *filename;         /* Do not free this. */

  /* If either line_no or code_point_no is zero, it means that field
     should be ignored. */
  size_t line_no;               /* Starting at 1. */
  size_t code_point_no;         /* Starting at 1, after canonicalization. */
};
typedef const struct text_location *text_location_t;

/* Some string constants. */
HIHA_PURE const string_t string_t_EOF (void);   /* “EOF” */
HIHA_PURE const string_t string_t_CP (void);    /* “CP” (code point) */
HIHA_PURE const string_t string_t_formfeed (void);      /* "\014" */

int string_t_cmp (string_t str1, string_t str2);

string_t make_string_t (const char *src);
string_t copy_string_t (string_t str);
string_t concat_string_t (...); /* The sentinel argument is NULL. */
char *make_str_nul (string_t str);

string_t string_t_from_str_len (const char *src, size_t srclen,
                                text_location_t loc);
string_t string_t_canonicalize (const string_t src,
                                text_location_t loc);
string_t string_t_canonical_from_str_len (const char *src,
                                          size_t srclen,
                                          text_location_t loc);

void str_len_from_string_t (const string_t src, char **s, size_t *n);

char *text_location_string (text_location_t);

void print_string_t (const string_t str, FILE *f);

/*--------------------------------------------------------------------*/

struct string_t_keyval
{
  string_t key;
  const void *value;
};
typedef const struct string_t_keyval *string_t_keyval_t;

DECLARE_HIHA_PERSISTENT_VECTOR_DATATYPE (, string_t_vector,
                                         string_t, 5);
DECLARE_HIHA_PERSISTENT_VECTOR_DATATYPE (, string_t_keyval_vector,
                                         string_t_keyval_t, 5);

/*--------------------------------------------------------------------*/

struct string_t_hash_context;
typedef struct string_t_hash_context *string_t_hash_context_t;

string_t_hash_context_t string_t_hash_init (string_t str);
uint64_t string_t_hash (string_t_hash_context_t context,
                        unsigned int i);

/*--------------------------------------------------------------------*/
/* Persistent unordered maps. */

struct string_t_map;
typedef const struct string_t_map *string_t_map_t;

size_t string_t_map_size (string_t_map_t map);

const void *string_t_map_search (string_t_map_t map, string_t key);

string_t_map_t string_t_map_insert_or_replace (string_t_map_t map,
                                               string_t key,
                                               const void *value);
string_t_map_t string_t_map_insert_only (string_t_map_t map,
                                         string_t key,
                                         const void *value);
string_t_map_t string_t_map_replace_only (string_t_map_t map,
                                          string_t key,
                                          const void *value);

string_t_map_t string_t_map_delete (string_t_map_t map, string_t key);

string_t_vector_t string_t_map_keys (string_t_map_t map);
voidp_vector_t string_t_map_values (string_t_map_t map);
string_t_keyval_vector_t string_t_map_associations (string_t_map_t map);

/*--------------------------------------------------------------------*/
/* Persistent ordered maps. */

struct string_t_omap;
typedef const struct string_t_omap *string_t_omap_t;

size_t string_t_omap_size (string_t_omap_t omap);

const void *string_t_omap_search (string_t_omap_t omap, string_t key);

/* string_t_omap_init creates a new empty map with a custom order.
   (The default order, which is string_t_cmp applied to the keys, is
   obtained by using NULL as the empty map.) */
string_t_omap_t string_t_omap_init (int (*compare) (string_t_keyval_t,
                                                    string_t_keyval_t));

string_t_omap_t string_t_omap_insert_or_replace (string_t_omap_t omap,
                                                 string_t key,
                                                 const void *value);
string_t_omap_t string_t_omap_insert_only (string_t_omap_t omap,
                                           string_t key,
                                           const void *value);
string_t_omap_t string_t_omap_replace_only (string_t_omap_t omap,
                                            string_t key,
                                            const void *value);

string_t_omap_t string_t_omap_delete (string_t_omap_t omap,
                                      string_t key);

string_t_vector_t string_t_omap_keys (string_t_omap_t omap);
voidp_vector_t string_t_omap_values (string_t_omap_t omap);
string_t_keyval_vector_t string_t_omap_associations (string_t_omap_t
                                                     omap);

/*--------------------------------------------------------------------*/

#endif /* __LIBHAHA__STRING_T_H__INCLUDED__ */

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
