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

#ifndef __LIBHAHA__STR_NUL_H__INCLUDED__
#define __LIBHAHA__STR_NUL_H__INCLUDED__

/* NUL-terminated strings of char. */

#include <string.h>
#include <libhiha/persistent_vector.h>

/*--------------------------------------------------------------------*/

inline bool
str_nul_equal (const char *a, const char *b)
{
  const bool a_is_null = (a == NULL);
  const bool b_is_null = (b == NULL);
  return ((a_is_null && b_is_null)
          || (!a_is_null && !b_is_null && strcmp (a, b) == 0));
}

/*--------------------------------------------------------------------*/

struct str_nul_keyval
{
  const char *key;
  const void *value;
};
typedef const struct str_nul_keyval *str_nul_keyval_t;

DECLARE_HIHA_PERSISTENT_VECTOR_DATATYPE (, str_nul_vector,
                                         const char *, 5);
DECLARE_HIHA_PERSISTENT_VECTOR_DATATYPE (, str_nul_keyval_vector,
                                         str_nul_keyval_t, 5);

/*--------------------------------------------------------------------*/

struct str_nul_hash_context;
typedef struct str_nul_hash_context *str_nul_hash_context_t;

str_nul_hash_context_t str_nul_hash_init (const char *);
uint64_t str_nul_hash (str_nul_hash_context_t context, unsigned int i);

/*--------------------------------------------------------------------*/
/*

  Persistent unordered maps.

  Be sure string keys you insert are not ephemeral or volatile. For
  instance they may be global constants or copies made with xstrdup().

  The NULL pointer IS allowed as a key.

*/

struct str_nul_map;
typedef const struct str_nul_map *str_nul_map_t;

size_t str_nul_map_size (str_nul_map_t map);

const void *str_nul_map_search (str_nul_map_t map, const char *key);

str_nul_map_t str_nul_map_insert_or_replace (str_nul_map_t map,
                                             const char *key,
                                             const void *value);
str_nul_map_t str_nul_map_insert_only (str_nul_map_t map,
                                       const char *key,
                                       const void *value);
str_nul_map_t str_nul_map_replace_only (str_nul_map_t map,
                                        const char *key,
                                        const void *value);

str_nul_map_t str_nul_map_delete (str_nul_map_t map, const char *key);

str_nul_vector_t str_nul_map_keys (str_nul_map_t map);
voidp_vector_t str_nul_map_values (str_nul_map_t map);
str_nul_keyval_vector_t str_nul_map_associations (str_nul_map_t map);

/*--------------------------------------------------------------------*/
/*

  Persistent unordered sets.

  Be sure strings you insert are not ephemeral or volatile. For
  instance they may be global constants or copies made with xstrdup().

  The NULL pointer IS allowed as an element.

*/

struct str_nul_set;
typedef const struct str_nul_set *str_nul_set_t;

size_t str_nul_set_size (str_nul_set_t set);

bool str_nul_set_contains (str_nul_set_t set, const char *element);
str_nul_set_t str_nul_set_adjoin (str_nul_set_t set,
                                  const char *element);
str_nul_set_t str_nul_set_delete (str_nul_set_t set,
                                  const char *element);

extern str_nul_set_t const str_nul_set_end;     /* Sentinel for “...” */

str_nul_set_t str_nul_set_union (...);
str_nul_set_t str_nul_set_intersection (...);
str_nul_set_t str_nul_set_difference (str_nul_set_t set, ...);
str_nul_set_t str_nul_set_symmetric_difference (str_nul_set_t set1,
                                                str_nul_set_t set2);

bool str_nul_set_equal (str_nul_set_t set, ...);
bool str_nul_set_subset (str_nul_set_t set, ...);
bool str_nul_set_proper_subset (str_nul_set_t set, ...);
bool str_nul_set_superset (str_nul_set_t set, ...);
bool str_nul_set_proper_superset (str_nul_set_t set, ...);

str_nul_vector_t str_nul_set_elements (str_nul_set_t set);

/*--------------------------------------------------------------------*/
/*

  Persistent ordered maps.

  Be sure string keys you insert are not ephemeral or volatile. For
  instance they may be global constants or copies made with xstrdup().

  The NULL pointer is allowed as a key ONLY if you use a custom
  “compare” function that can handle it. The default “compare” is
  strcmp(3) applied to the keys, which CANNOT handle the NULL pointer.

*/

struct str_nul_omap;
typedef const struct str_nul_omap *str_nul_omap_t;

size_t str_nul_omap_size (str_nul_omap_t omap);

const void *str_nul_omap_search (str_nul_omap_t omap, const char *key);

/* str_nul_omap_init creates a new empty map with a custom order. (The
   default order, which is strcmp(3) applied to the keys, is obtained
   by using NULL as the empty map.) */
str_nul_omap_t str_nul_omap_init (int (*compare) (str_nul_keyval_t,
                                                  str_nul_keyval_t));

str_nul_omap_t str_nul_omap_insert_or_replace (str_nul_omap_t omap,
                                               const char *key,
                                               const void *value);
str_nul_omap_t str_nul_omap_insert_only (str_nul_omap_t omap,
                                         const char *key,
                                         const void *value);
str_nul_omap_t str_nul_omap_replace_only (str_nul_omap_t omap,
                                          const char *key,
                                          const void *value);

str_nul_omap_t str_nul_omap_delete (str_nul_omap_t omap,
                                    const char *key);

/* direction = 1 forwards, -1 backwards */
str_nul_vector_t str_nul_omap_keys (str_nul_omap_t omap, int direction);
voidp_vector_t str_nul_omap_values (str_nul_omap_t omap, int direction);
str_nul_keyval_vector_t str_nul_omap_associations (str_nul_omap_t omap,
                                                   int direction);

/*--------------------------------------------------------------------*/

#endif /* __LIBHAHA__STR_NUL_H__INCLUDED__ */

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
