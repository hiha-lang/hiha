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

#ifndef __LIBHAHA__PERSISTENT_HASH_MAP_H__INCLUDED__
#define __LIBHAHA__PERSISTENT_HASH_MAP_H__INCLUDED__

/*

  Ideal hash maps.

*/

#include <stdint.h>
#include <libhiha/persistent_integer_trie.h>

#ifndef HIHA_HASH_MAP_ALLOC
#include <xalloc.h>
#define HIHA_HASH_MAP_ALLOC(T) XMALLOC (T)
#endif

#define HIHA_HASH_MAP_DECL(NAME, VALTYPE)                       \
                                                                \
  HIHA_INT_TRIE_NODES_DECL (NAME##_node, uint64_t, VALTYPE);    \
                                                                \
  typedef struct NAME                                           \
  {                                                             \
    size_t size;                                                \
    uint64_t hash_function (void *hash_context,                 \
                            unsigned int hash_bits_index);      \
    bool equality_test (const VALTYPE *key_object,              \
                        const VALTYPE *stored_object);          \
    NAME##_node_t trie;                                         \
  } *NAME##_t

#endif /* __LIBHAHA__PERSISTENT_HASH_MAP_H__INCLUDED__ */

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
