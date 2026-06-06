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
/*

  This file is adapted from the Sorts Mill Core Library’s
  <sortsmill/core/immutable_vectors.h>, which is:

  Copyright (C) 2015 Khaled Hosny and Barry Schwartz

  Sorts Mill Core Library is free software; you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 3 of
  the License, or (at your option) any later version.
 
  Sorts Mill Core Library is distributed in the hope that it will be
  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.

*/


#ifndef __LIBHAHA__PERSISTENT_VECTOR_H__INCLUDED__
#define __LIBHAHA__PERSISTENT_VECTOR_H__INCLUDED__

/*

  The implementation uses ordinary tries with tails (as opposed to,
  for instance, RRB-trees).
  
  Reference:
  
  @mastersthesis{lorange2014rrb,
  author = {L'orange, Jean Niklas},
  title  = {{Improving RRB-Tree Performance through Transience}},
  school = {Norwegian University of Science and Technology},
  year   = {2014},
  month  = {June}}
  
  <http://hypirion.com/thesis>


  NOTE: On account of how Boehm GC works, we cannot play optimization
  tricks such as using the least significant bit of a pointer as a
  flag. Do not be tempted, even if you are willing to assume all the
  structures pointed to have alignments on even addresses.

*/

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#ifndef HIHA_VECTOR_ZALLOCSZ
#include <xalloc.h>
#define HIHA_VECTOR_ZALLOCSZ(sz) xzalloc (sz)
#endif

#ifndef HIHA_VECTOR_ZALLOC_ATOMICSZ
#include <libhiha/xalloc.h>
#define HIHA_VECTOR_ZALLOC_ATOMICSZ(sz) xzalloc_atomic (sz)
#endif

/*--------------------------------------------------------------------*/

/* Returns the length of a persistent vector of any entry type. */
inline size_t hiha_persistent_vector_length (void *pvect);
inline size_t
hiha_persistent_vector_length (void *pvect)
{
  typedef struct
  {
    size_t length;
  } pvect_t;
  return (pvect == (void *) 0) ? 0 : (((pvect_t *) pvect)->length);
}

#define DECLARE_HIHA_PERSISTENT_VECTOR_T(MODIFIER, T, ENTRY_T, BITS)    \
                                                                        \
  typedef struct                                                        \
  {                                                                     \
    void *a[((size_t) 1) << (BITS)];                                    \
  } T##__internal__;                                                    \
                                                                        \
  typedef struct                                                        \
  {                                                                     \
    ENTRY_T a[((size_t) 1) << (BITS)];                                  \
  } T##__leaf__;                                                        \
                                                                        \
  typedef struct                                                        \
  {                                                                     \
    size_t length;                                                      \
    unsigned char shift;                                                \
    void *node;                                                         \
    T##__leaf__ *tail;                                                  \
  } T##__vector__;                                                      \
                                                                        \
  typedef T##__vector__ *T;                                             \
                                                                        \
  MODIFIER inline size_t T##__tail_length0__ (T);                       \
  MODIFIER inline size_t                                                \
  T##__tail_length0__ (T vec)                                           \
  {                                                                     \
    /* Return zero if the tail is full;  */                             \
    /* otherwise return the tail length. */                             \
    return vec->length & ((((size_t) 1) << (BITS)) - 1);                \
  }                                                                     \
                                                                        \
  MODIFIER inline size_t T##__tail_length__ (T);                        \
  MODIFIER inline size_t                                                \
  T##__tail_length__ (T vec)                                            \
  {                                                                     \
    const size_t n = vec->length & ((((size_t) 1) << (BITS)) - 1);      \
    return (n == 0) ? (((size_t) 1) << (BITS)) : n;                     \
  }
/*                                               */
#if !CHECKING_PERSISTENT_VECTOR_STRUCTURE__     /*   */
/*                                               */
#define DEFINE_HIHA_PERSISTENT_VECTOR_T(MODIFIER, T, ENTRY_T, BITS)     \
  MODIFIER size_t T##__tail_length0__ (T);                              \
  MODIFIER size_t T##__tail_length__ (T);                               \
  static inline void T##__check_structure__ (T vec)                     \
  {                                                                     \
    /* Do nothing. */                                                   \
  }
/*                                               */
#else /* CHECKING_PERSISTENT_VECTOR_STRUCTURE__  */
/*                                               */
#define DEFINE_HIHA_PERSISTENT_VECTOR_T(MODIFIER, T, ENTRY_T, BITS)     \
  MODIFIER size_t T##__tail_length0__ (T);                              \
  MODIFIER size_t T##__tail_length__ (T);                               \
  static void                                                           \
  T##__check_fully_dense__ (void *p, size_t s)                          \
  {                                                                     \
    if (s != 0)                                                         \
      {                                                                 \
        T##__internal__ *const q = p;                                   \
        /* Walk the tree. All entries should be non-NULL. */            \
        for (size_t k = 0; k < (((size_t) 1) << (BITS)); k++)           \
          {                                                             \
            assert (q->a[k] != (void *) 0);                             \
            T##__check_fully_dense__ (q->a[k], s - (BITS));             \
          }                                                             \
      }                                                                 \
  }                                                                     \
  static void                                                           \
  T##__check_structure__ (T vec)                                        \
  {                                                                     \
    if (vec != NULL)                                                    \
      {                                                                 \
        assert (vec->length != 0);                                      \
                                                                        \
        const size_t mask = ((((size_t) 1) << (BITS)) - 1);             \
        const size_t tail_length = T##__tail_length__ (vec);            \
                                                                        \
        /* Check the tail. */                                           \
        assert (tail_length <= vec->length);                            \
        assert (vec->tail != (T##__leaf__ *) 0);                        \
                                                                        \
        /* Check the trie. */                                           \
        if (tail_length == vec->length)                                 \
          assert (vec->node == (void *) 0);                             \
        else                                                            \
          {                                                             \
            /* The last entry in the trie. */                           \
            const size_t i = vec->length - tail_length - 1;             \
                                                                        \
            void *p = vec->node;                                        \
            for (size_t s = vec->shift; s != 0; s -= (BITS))            \
              {                                                         \
                const size_t index = (i >> s) & mask;                   \
                T##__internal__ *const q = p;                           \
                for (size_t k = 0; k < index; k++)                      \
                  {                                                     \
                    assert (q->a[k] != (void *) 0);                     \
                    T##__check_fully_dense__ (q->a[k], s - (BITS));     \
                  }                                                     \
                assert (q->a[index] != (void *) 0);                     \
                for (size_t k = index + 1; k <= mask; k++)              \
                  assert (q->a[k] == (void *) 0);                       \
                p = q->a[index];                                        \
              }                                                         \
          }                                                             \
      }                                                                 \
  }
/*                                               */
#endif /* CHECKING_PERSISTENT_VECTOR_STRUCTURE__ */
/*                                               */

#define DECLARE_HIHA_PERSISTENT_VECTOR_LENGTH(MODIFIER, NAME, T)        \
  MODIFIER inline size_t NAME (T);                                      \
  MODIFIER inline size_t                                                \
  NAME (T vec)                                                          \
  {                                                                     \
    return (vec == (void *) 0) ? 0 : vec->length;                       \
  }
#define DEFINE_HIHA_PERSISTENT_VECTOR_LENGTH(MODIFIER, NAME, T) \
  MODIFIER size_t NAME (T);

#define DECLARE_HIHA_PERSISTENT_VECTOR_REF(MODIFIER, NAME, T, ENTRY_T,  \
                                           BITS)                        \
  MODIFIER inline ENTRY_T NAME (T, size_t);                             \
  MODIFIER inline ENTRY_T                                               \
  NAME (T vec, size_t i)                                                \
  {                                                                     \
    assert (vec != (void *) 0 && i < vec->length);                      \
    ENTRY_T value;                                                      \
    const size_t tail_length = T##__tail_length__ (vec);                \
    if (vec->length - tail_length <= i)                                 \
      {                                                                 \
        /* The desired entry is in the tail. */                         \
        value = vec->tail->a[i - (vec->length - tail_length)];          \
      }                                                                 \
    else                                                                \
      {                                                                 \
        const size_t mask = (((size_t) 1) << (BITS)) - 1;               \
        void *p = vec->node;                                            \
        for (size_t s = vec->shift; s != 0; s -= (BITS))                \
          {                                                             \
            T##__internal__ *const q = p;                               \
            p = q->a[(i >> s) & mask];                                  \
          }                                                             \
        value = ((T##__leaf__ *) p)->a[i & mask];                       \
      }                                                                 \
    return value;                                                       \
  }
#define DEFINE_HIHA_PERSISTENT_VECTOR_REF(MODIFIER, NAME, T, ENTRY_T,   \
                                          BITS)                         \
  MODIFIER ENTRY_T NAME (T, size_t);

#define DECLARE_HIHA_PERSISTENT_VECTOR_PTR(MODIFIER, NAME, T, ENTRY_T,  \
                                           BITS)                        \
  MODIFIER inline const ENTRY_T *NAME (T, size_t);                      \
  MODIFIER inline const ENTRY_T *                                       \
  NAME (T vec, size_t i)                                                \
  {                                                                     \
    const ENTRY_T *ptr;                                                 \
    if (vec != (void *) 0 && i < vec->length)                           \
      {                                                                 \
        const size_t tail_length = T##__tail_length__ (vec);            \
        if (vec->length - tail_length <= i)                             \
          {                                                             \
            /* The desired entry is in the tail. */                     \
            ptr = &vec->tail->a[i - (vec->length - tail_length)];       \
          }                                                             \
        else                                                            \
          {                                                             \
            const size_t mask = (((size_t) 1) << (BITS)) - 1;           \
            void *p = vec->node;                                        \
            for (size_t s = vec->shift; s != 0; s -= (BITS))            \
              {                                                         \
                T##__internal__ *const q = p;                           \
                p = q->a[(i >> s) & mask];                              \
              }                                                         \
            ptr = &((T##__leaf__ *) p)->a[i & mask];                    \
          }                                                             \
      }                                                                 \
    else                                                                \
      ptr = (const ENTRY_T *) 0;                                        \
    return ptr;                                                         \
  }
#define DEFINE_HIHA_PERSISTENT_VECTOR_PTR(MODIFIER, NAME, T, ENTRY_T,   \
                                          BITS)                         \
  MODIFIER const ENTRY_T *NAME (T, size_t);

#define DECLARE_HIHA_PERSISTENT_VECTOR_NEXT(MODIFIER, NAME, T, ENTRY_T, \
                                            BITS)                       \
  MODIFIER inline const ENTRY_T *NAME (T, size_t, const ENTRY_T *);     \
  MODIFIER inline const ENTRY_T *                                       \
  NAME (T vec, size_t i, const ENTRY_T *current)                        \
  {                                                                     \
    const ENTRY_T *next;                                                \
    if (vec != (void *) 0 && i < vec->length - 1)                       \
      {                                                                 \
        i++;                                                            \
        const size_t mask = (((size_t) 1) << (BITS)) - 1;               \
        if ((i & mask) == 0)                                            \
          {                                                             \
            const size_t tail_length = T##__tail_length__ (vec);        \
            if (vec->length - tail_length <= i)                         \
              {                                                         \
                /* The desired entry is in the tail. */                 \
                next = &vec->tail->a[i - (vec->length - tail_length)];  \
              }                                                         \
            else                                                        \
              {                                                         \
                void *p = vec->node;                                    \
                for (size_t s = vec->shift; s != 0; s -= (BITS))        \
                  {                                                     \
                    T##__internal__ *const q = p;                       \
                    p = q->a[(i >> s) & mask];                          \
                  }                                                     \
                next = &((T##__leaf__ *) p)->a[0];                      \
              }                                                         \
          }                                                             \
        else                                                            \
          next = current + 1;                                           \
      }                                                                 \
    else                                                                \
      next = (const ENTRY_T *) 0;                                       \
    return next;                                                        \
  }
#define DEFINE_HIHA_PERSISTENT_VECTOR_NEXT(MODIFIER, NAME, T, ENTRY_T,  \
                                           BITS)                        \
  MODIFIER const ENTRY_T *NAME (T, size_t, const ENTRY_T *);

#define DECLARE_HIHA_PERSISTENT_VECTOR_PREV(MODIFIER, NAME, T, ENTRY_T, \
                                            BITS)                       \
  MODIFIER inline const ENTRY_T *NAME (T, size_t, const ENTRY_T *);     \
  MODIFIER inline const ENTRY_T *                                       \
  NAME (T vec, size_t i, const ENTRY_T *current)                        \
  {                                                                     \
    const ENTRY_T *prev;                                                \
    if (vec == (void *) 0 || i == 0)                                    \
      prev = (const ENTRY_T *) 0;                                       \
    else                                                                \
      {                                                                 \
        const size_t mask = (((size_t) 1) << (BITS)) - 1;               \
        if ((i & mask) == 0)                                            \
          {                                                             \
            i--;                                                        \
            void *p = vec->node;                                        \
            for (size_t s = vec->shift; s != 0; s -= (BITS))            \
              {                                                         \
                T##__internal__ *const q = p;                           \
                p = q->a[(i >> s) & mask];                              \
              }                                                         \
            prev = &((T##__leaf__ *) p)->a[i & mask];                   \
          }                                                             \
        else                                                            \
          prev = current - 1;                                           \
      }                                                                 \
    return prev;                                                        \
  }
#define DEFINE_HIHA_PERSISTENT_VECTOR_PREV(MODIFIER, NAME, T, ENTRY_T,  \
                                           BITS)                        \
  MODIFIER const ENTRY_T *NAME (T, size_t, const ENTRY_T *);

/* FIXME: Rewrite REFS to have tighter loops. */
#define DECLARE_HIHA_PERSISTENT_VECTOR_REFS(MODIFIER, NAME, T,  \
                                            ENTRY_T, BITS)      \
  MODIFIER void NAME (T, size_t, size_t n, ENTRY_T x[n]);
#define DEFINE_HIHA_PERSISTENT_VECTOR_REFS(MODIFIER, NAME, T,           \
                                           ENTRY_T, BITS,               \
                                           PVECT_PTR, PVECT_NEXT)       \
  MODIFIER void                                                         \
  NAME (T vec, size_t i, size_t n, ENTRY_T x[n])                        \
  {                                                                     \
    assert ((vec == NULL) ? (i + n == 0) : (i + n <= vec->length));     \
    const ENTRY_T *p = PVECT_PTR (vec, i);                              \
    size_t k = 0;                                                       \
    while (k < n)                                                       \
      {                                                                 \
        x[k] = *p;                                                      \
        p = PVECT_NEXT (vec, i + k, p);                                 \
        k++;                                                            \
      }                                                                 \
  }

#define DECLARE_HIHA_PERSISTENT_VECTOR_SET(MODIFIER, NAME, T, ENTRY_T,  \
                                           BITS)                        \
  MODIFIER T NAME (T, size_t, ENTRY_T);
#define DEFINE_HIHA_PERSISTENT_VECTOR_SET(MODIFIER, NAME, T, ENTRY_T,   \
                                          BITS,                         \
                                          POINTERS_MALLOC,              \
                                          ENTRIES_MALLOC)               \
  MODIFIER T                                                            \
  NAME (T vec, size_t i, ENTRY_T x)                                     \
  {                                                                     \
    assert (vec != (void *) 0 && i < vec->length);                      \
                                                                        \
    /* Make a new root. */                                              \
    T##__vector__ *const vec_ =                                         \
      POINTERS_MALLOC (sizeof (T##__vector__));                         \
    *vec_ = *vec;                                                       \
    void **parent = &vec_->node;                                        \
                                                                        \
    const size_t tail_length = T##__tail_length__ (vec_);               \
    if (vec_->length - tail_length <= i)                                \
      {                                                                 \
        /* The desired entry is in the tail. Make a new tail. */        \
        T##__leaf__ *const entries =                                    \
          ENTRIES_MALLOC (sizeof (T##__leaf__));                        \
        *entries = *vec_->tail;                                         \
        vec_->tail = entries;                                           \
        entries->a[i - (vec_->length - tail_length)] = x;               \
      }                                                                 \
    else                                                                \
      {                                                                 \
        /* Make new internal nodes. */                                  \
        void *p = vec_->node;                                           \
        for (size_t s = vec_->shift; s != 0; s -= (BITS))               \
          {                                                             \
            T##__internal__ *const q =                                  \
              POINTERS_MALLOC (sizeof (T##__internal__));               \
            *q = *(T##__internal__ *) p;                                \
            *parent = q;                                                \
            parent = &q->a[(i >> s) & ((((size_t) 1) << (BITS)) - 1)];  \
            p = *parent;                                                \
          }                                                             \
                                                                        \
        /* Make a new leaf node. */                                     \
        T##__leaf__ *const entries =                                    \
          ENTRIES_MALLOC (sizeof (T##__leaf__));                        \
        *entries = *(T##__leaf__ *) p;                                  \
        entries->a[i & ((((size_t) 1) << (BITS)) - 1)] = x;             \
        *parent = entries;                                              \
      }                                                                 \
                                                                        \
    T##__check_structure__ (vec_);                                      \
                                                                        \
    return vec_;                                                        \
  }

#define DECLARE_HIHA_PERSISTENT_VECTOR_SETS(MODIFIER, NAME, T,  \
                                            ENTRY_T, BITS)      \
  MODIFIER T NAME (T, size_t, size_t n, const ENTRY_T x[n]);
#define DEFINE_HIHA_PERSISTENT_VECTOR_SETS(MODIFIER, NAME, T, ENTRY_T,  \
                                           BITS,                        \
                                           POINTERS_MALLOC,             \
                                           ENTRIES_MALLOC)              \
  MODIFIER T                                                            \
  NAME (T vec, size_t i, size_t n, const ENTRY_T x[n])                  \
  {                                                                     \
    if (0 < n)                                                          \
      {                                                                 \
        assert (vec != (void *) 0);                                     \
        assert (n <= vec->length);                                      \
        assert (i <= vec->length - n);                                  \
                                                                        \
        const size_t end = i + n;                                       \
        const ENTRY_T *chunk = x;                                       \
        while (i != end)                                                \
          {                                                             \
            size_t j =                                                  \
              ((i + (((size_t) 1) << (BITS))) / (((size_t) 1) << (BITS))) * \
              (((size_t) 1) << (BITS));                                 \
            if (end < j)                                                \
              j = end;                                                  \
                                                                        \
            /* Make a new root. */                                      \
            T##__vector__ *const vec_ =                                 \
              POINTERS_MALLOC (sizeof (T##__vector__));                 \
            *vec_ = *vec;                                               \
            void **parent = &vec_->node;                                \
                                                                        \
            const size_t tail_length = T##__tail_length__ (vec_);       \
            if (vec_->length - tail_length <= i)                        \
              {                                                         \
                /* The desired entries are in the tail. */              \
                /* Make a new tail.                     */              \
                T##__leaf__ *const entries =                            \
                  ENTRIES_MALLOC (sizeof (T##__leaf__));                \
                *entries = *vec_->tail;                                 \
                vec_->tail = entries;                                   \
                memcpy (&entries->a[i - (vec_->length - tail_length)],  \
                        chunk, (j - i) * sizeof (ENTRY_T));             \
              }                                                         \
            else                                                        \
              {                                                         \
                /* Make new internal nodes. */                          \
                void *p = vec_->node;                                   \
                for (size_t s = vec_->shift; s != 0; s -= (BITS))       \
                  {                                                     \
                    T##__internal__ *const q =                          \
                      POINTERS_MALLOC (sizeof (T##__internal__));       \
                    *q = *(T##__internal__ *) p;                        \
                    *parent = q;                                        \
                    parent =                                            \
                      &q->a[(i >> s) & ((((size_t) 1) << (BITS)) - 1)]; \
                    p = *parent;                                        \
                  }                                                     \
                                                                        \
                /* Make a new leaf node. */                             \
                T##__leaf__ *const entries =                            \
                  ENTRIES_MALLOC (sizeof (T##__leaf__));                \
                *entries = *(T##__leaf__ *) p;                          \
                memcpy (&entries->a[i & ((((size_t) 1) << (BITS)) - 1)], \
                        chunk, (j - i) * sizeof (ENTRY_T));             \
                *parent = entries;                                      \
              }                                                         \
                                                                        \
            vec = vec_;                                                 \
            chunk += j - i;                                             \
            i = j;                                                      \
          }                                                             \
                                                                        \
        T##__check_structure__ (vec);                                   \
      }                                                                 \
                                                                        \
    return vec;                                                         \
  }

#define DECLARE_HIHA_PERSISTENT_VECTOR_PUSH(MODIFIER, NAME, T,  \
                                            ENTRY_T, BITS)      \
  MODIFIER T NAME (T, ENTRY_T);
#define DEFINE_HIHA_PERSISTENT_VECTOR_PUSH(MODIFIER, NAME, T,           \
                                           ENTRY_T, BITS,               \
                                           POINTERS_MALLOC,             \
                                           ENTRIES_MALLOC)              \
  MODIFIER T                                                            \
  NAME (T vec, ENTRY_T x)                                               \
  {                                                                     \
    /* Make a new root. */                                              \
    T##__vector__ *const vec_ =                                         \
      POINTERS_MALLOC (sizeof (T##__vector__));                         \
                                                                        \
    if (vec == (void *) 0)                                              \
      {                                                                 \
        /* Start a new vector. */                                       \
                                                                        \
        vec_->length = 1;                                               \
        vec_->shift = 0;                                                \
        T##__leaf__ *const entries =                                    \
          ENTRIES_MALLOC (sizeof (T##__leaf__));                        \
        vec_->tail = entries;                                           \
        entries->a[0] = x;                                              \
      }                                                                 \
    else                                                                \
      {                                                                 \
        const size_t tail_length = T##__tail_length0__ (vec);           \
                                                                        \
        if (tail_length != 0)                                           \
          {                                                             \
            /* Extend the tail. */                                      \
                                                                        \
            vec_->length = vec->length + 1;                             \
            vec_->shift = vec->shift;                                   \
            vec_->node = vec->node;                                     \
            T##__leaf__ *const entries =                                \
              ENTRIES_MALLOC (sizeof (T##__leaf__));                    \
            vec_->tail = entries;                                       \
            *entries = *vec->tail;                                      \
            entries->a[tail_length] = x;                                \
          }                                                             \
        else                                                            \
          {                                                             \
            /* The tail is full. Push it and start a new one. */        \
                                                                        \
            void **parent;                                              \
                                                                        \
            vec_->length = vec->length + 1;                             \
                                                                        \
            const size_t i = vec->length - (((size_t) 1) << (BITS));    \
            if (i == 0)                                                 \
              {                                                         \
                /* Start a new trie. */                                 \
                vec_->node = vec->tail;                                 \
                T##__leaf__ *const entries =                            \
                  ENTRIES_MALLOC (sizeof (T##__leaf__));                \
                vec_->tail = entries;                                   \
                entries->a[0] = x;                                      \
              }                                                         \
            else                                                        \
              {                                                         \
                if (i == (((size_t) 1) << (vec->shift + (BITS))))       \
                  {                                                     \
                    /* The trie is fully dense. Raise its height. */    \
                    vec_->shift = vec->shift + (BITS);                  \
                    T##__internal__ *const new_node =                   \
                      POINTERS_MALLOC (sizeof (T##__internal__));       \
                    new_node->a[0] = vec->node;                         \
                    vec_->node = new_node;                              \
                    parent = &new_node->a[1];                           \
                  }                                                     \
                else                                                    \
                  {                                                     \
                    vec_->shift = vec->shift;                           \
                    vec_->node = vec->node;                             \
                    parent = &vec_->node;                               \
                  }                                                     \
                                                                        \
                /* Make new internal nodes. */                          \
                void *p = *parent;                                      \
                for (size_t s = vec->shift; s != 0; s -= (BITS))        \
                  {                                                     \
                    T##__internal__ *const q =                          \
                      POINTERS_MALLOC (sizeof (T##__internal__));       \
                    if (p != (void *) 0)                                \
                      *q = *(T##__internal__ *) p;                      \
                    *parent = q;                                        \
                    parent =                                            \
                      &q->a[(i >> s) & ((((size_t) 1) << (BITS)) - 1)]; \
                    p = *parent;                                        \
                  }                                                     \
                                                                        \
                /* Push the tail. */                                    \
                *parent = vec->tail;                                    \
                                                                        \
                /* Start a new tail. */                                 \
                T##__leaf__ *const entries =                            \
                  ENTRIES_MALLOC (sizeof (T##__leaf__));                \
                vec_->tail = entries;                                   \
                entries->a[0] = x;                                      \
              }                                                         \
          }                                                             \
      }                                                                 \
                                                                        \
    T##__check_structure__ (vec_);                                      \
                                                                        \
    return vec_;                                                        \
  }

#define DECLARE_HIHA_PERSISTENT_VECTOR_PUSHES(MODIFIER, NAME, T,        \
                                              ENTRY_T, BITS)            \
  MODIFIER T NAME (T, size_t n, const ENTRY_T x[n]);
#define DEFINE_HIHA_PERSISTENT_VECTOR_PUSHES(MODIFIER, NAME, T,         \
                                             ENTRY_T, BITS,             \
                                             POINTERS_MALLOC,           \
                                             ENTRIES_MALLOC)            \
  MODIFIER T                                                            \
  NAME (T vec, size_t n, const ENTRY_T x[n])                            \
  {                                                                     \
    const ENTRY_T *chunk = x;                                           \
    while (n != 0)                                                      \
      {                                                                 \
        const size_t chunk_size_max = (vec == (void *) 0) ?             \
          (((size_t) 1) << (BITS)) :                                    \
          ((((size_t) 1) << (BITS)) - T##__tail_length0__ (vec));       \
        const size_t chunk_size =                                       \
          (n < chunk_size_max) ? n : chunk_size_max;                    \
                                                                        \
        /* Make a new root. */                                          \
        T##__vector__ *const vec_ =                                     \
          POINTERS_MALLOC (sizeof (T##__vector__));                     \
                                                                        \
        size_t offset;                                                  \
                                                                        \
        if (vec == (void *) 0)                                          \
          {                                                             \
            /* Start a new vector. */                                   \
                                                                        \
            vec_->length = chunk_size;                                  \
            vec_->shift = 0;                                            \
            T##__leaf__ *const entries =                                \
              ENTRIES_MALLOC (sizeof (T##__leaf__));                    \
            vec_->tail = entries;                                       \
            offset = 0;                                                 \
          }                                                             \
        else                                                            \
          {                                                             \
            const size_t tail_length = T##__tail_length0__ (vec);       \
                                                                        \
            if (tail_length != 0)                                       \
              {                                                         \
                /* Extend the tail. */                                  \
                                                                        \
                vec_->length = vec->length + chunk_size;                \
                vec_->shift = vec->shift;                               \
                vec_->node = vec->node;                                 \
                T##__leaf__ *const entries =                            \
                  ENTRIES_MALLOC (sizeof (T##__leaf__));                \
                vec_->tail = entries;                                   \
                *entries = *vec->tail;                                  \
                offset = tail_length;                                   \
              }                                                         \
            else                                                        \
              {                                                         \
                /* The tail is full. Push it and start a new one. */    \
                                                                        \
                void **parent;                                          \
                                                                        \
                vec_->length = vec->length + chunk_size;                \
                                                                        \
                const size_t i =                                        \
                  vec->length - (((size_t) 1) << (BITS));               \
                if (i == 0)                                             \
                  {                                                     \
                    /* Start a new trie. */                             \
                    vec_->node = vec->tail;                             \
                    T##__leaf__ *const entries =                        \
                      ENTRIES_MALLOC (sizeof (T##__leaf__));            \
                    vec_->tail = entries;                               \
                    offset = 0;                                         \
                  }                                                     \
                else                                                    \
                  {                                                     \
                    if (i == (((size_t) 1) << (vec->shift + (BITS))))   \
                      {                                                 \
                        /* The trie is fully dense. */                  \
                        /* Raise its height.        */                  \
                        vec_->shift = vec->shift + (BITS);              \
                        T##__internal__ *const new_node =               \
                          POINTERS_MALLOC (sizeof (T##__internal__));   \
                        new_node->a[0] = vec->node;                     \
                        vec_->node = new_node;                          \
                        parent = &new_node->a[1];                       \
                      }                                                 \
                    else                                                \
                      {                                                 \
                        vec_->shift = vec->shift;                       \
                        vec_->node = vec->node;                         \
                        parent = &vec_->node;                           \
                      }                                                 \
                                                                        \
                    /* Make new internal nodes. */                      \
                    const size_t mask = ((((size_t) 1) << (BITS)) - 1); \
                    void *p = *parent;                                  \
                    for (size_t s = vec->shift; s != 0; s -= (BITS))    \
                      {                                                 \
                        T##__internal__ *const q =                      \
                          POINTERS_MALLOC (sizeof (T##__internal__));   \
                        if (p != (void *) 0)                            \
                          *q = *(T##__internal__ *) p;                  \
                        *parent = q;                                    \
                        parent = &q->a[(i >> s) & mask];                \
                        p = *parent;                                    \
                      }                                                 \
                                                                        \
                    /* Push the tail. */                                \
                    *parent = vec->tail;                                \
                                                                        \
                    /* Start a new tail. */                             \
                    T##__leaf__ *const entries =                        \
                      ENTRIES_MALLOC (sizeof (T##__leaf__));            \
                    vec_->tail = entries;                               \
                    offset = 0;                                         \
                  }                                                     \
              }                                                         \
          }                                                             \
                                                                        \
        T##__leaf__ *const entries = vec_->tail;                        \
        for (size_t i = 0; i < chunk_size; i++)                         \
          entries->a[offset + i] = chunk[i];                            \
                                                                        \
        /* Get ready for the next chunk of new entries. */              \
        vec = vec_;                                                     \
        chunk += chunk_size;                                            \
        n -= chunk_size;                                                \
      }                                                                 \
                                                                        \
    T##__check_structure__ (vec);                                       \
                                                                        \
    return vec;                                                         \
  }

#define DECLARE_HIHA_PERSISTENT_VECTOR_POP(MODIFIER, NAME, T, BITS)     \
  MODIFIER T NAME (T);
#define DEFINE_HIHA_PERSISTENT_VECTOR_POP(MODIFIER, NAME, T, BITS,      \
                                          POINTERS_MALLOC,              \
                                          ENTRIES_MALLOC,               \
                                          COLLECT_ENTRIES)              \
  MODIFIER T                                                            \
  NAME (T vec)                                                          \
  {                                                                     \
    assert (vec != (void *) 0);                                         \
                                                                        \
    T##__vector__ *vec_;                                                \
                                                                        \
    if (vec->length == 1)                                               \
      /* The new vector is empty; our representation for */             \
      /* an empty vector is a NULL pointer.              */             \
      vec_ = (T##__vector__ *) 0;                                       \
    else                                                                \
      {                                                                 \
        /* Make a new root. */                                          \
        vec_ = POINTERS_MALLOC (sizeof (T##__vector__));                \
        *vec_ = *vec;                                                   \
        vec_->length--;                                                 \
                                                                        \
        const size_t tail_length = T##__tail_length__ (vec);            \
        if (tail_length == 1)                                           \
          {                                                             \
            /* We have to take the last block of the trie */            \
            /* and make it a new tail.                    */            \
                                                                        \
            const size_t mask = ((((size_t) 1) << (BITS)) - 1);         \
                                                                        \
            const size_t i = vec_->length - (((size_t) 1) << (BITS));   \
            if (i == 0)                                                 \
              {                                                         \
                /* All entries now are in the tail; */                  \
                /* the trie is empty.               */                  \
                vec_->tail = vec_->node;                                \
                vec_->node = (void *) 0;                                \
              }                                                         \
            else if (i == (((size_t) 1) << vec_->shift))                \
              {                                                         \
                /* Lower the height of the trie, yielding a */          \
                /* fully dense trie.                        */          \
                void *p = vec_->node;                                   \
                for (size_t s = vec_->shift; s != 0; s -= (BITS))       \
                  {                                                     \
                    T##__internal__ *const q = p;                       \
                    p = q->a[(i >> s) & mask];                          \
                  }                                                     \
                vec_->node = ((T##__internal__ *) vec_->node)->a[0];    \
                vec_->tail = p;                                         \
                vec_->shift -= (BITS);                                  \
              }                                                         \
            else                                                        \
              {                                                         \
                /* `pruning' will be set true if we can prune  */       \
                /* a branch of the trie.                       */       \
                bool pruning = false;                                   \
                                                                        \
                void **parent = &vec_->node;                            \
                void *p = *parent;                                      \
                for (size_t s = vec_->shift; s != 0; s -= (BITS))       \
                  {                                                     \
                    if (pruning)                                        \
                      {                                                 \
                        /* We have found the place at which      */     \
                        /* to put a NULL (`parent'), but must    */     \
                        /* continue searching for the new tail.  */     \
                        /* And we know that all the indices from */     \
                        /* now on are zero.                      */     \
                        T##__internal__ *const q = p;                   \
                        p = q->a[0];                                    \
                      }                                                 \
                    else                                                \
                      {                                                 \
                        /* Make a new internal node. */                 \
                        T##__internal__ *const q =                      \
                          POINTERS_MALLOC (sizeof (T##__internal__));   \
                        *q = *(T##__internal__ *) p;                    \
                        *parent = q;                                    \
                        parent = &q->a[(i >> s) & mask];                \
                        p = *parent;                                    \
                        pruning = (i == ((i >> s) << s));               \
                      }                                                 \
                  }                                                     \
                *parent = (void *) 0;                                   \
                vec_->tail = p;                                         \
              }                                                         \
          }                                                             \
        else if (COLLECT_ENTRIES)                                       \
          {                                                             \
            /* Clear the popped entry, so it can be collected */        \
            /* as garbage if not referenced elsewhere.        */        \
            T##__leaf__ *const entries =                                \
              ENTRIES_MALLOC (sizeof (T##__leaf__));                    \
            *entries = *vec_->tail;                                     \
            vec_->tail = entries;                                       \
            /* A fixed-length memset() that likely will be */           \
            /* expanded inline by the compiler.            */           \
            memset (&entries->a[tail_length - 1], 0,                    \
                    sizeof entries->a[tail_length - 1]);                \
          }                                                             \
      }                                                                 \
                                                                        \
    T##__check_structure__ (vec_);                                      \
                                                                        \
    return vec_;                                                        \
  }

#define DECLARE_HIHA_PERSISTENT_VECTOR_POPS(MODIFIER, NAME, T, BITS)    \
  MODIFIER T NAME (T, size_t);
#define DEFINE_HIHA_PERSISTENT_VECTOR_POPS(MODIFIER, NAME, T, BITS,     \
                                           POINTERS_MALLOC,             \
                                           ENTRIES_MALLOC,              \
                                           COLLECT_ENTRIES)             \
                                                                        \
  static inline void                                                    \
  NAME##__collect_entries__ (T##__vector__ *vec_,                       \
                             size_t new_tail_length)                    \
  {                                                                     \
    /* If the popped entries are atomic, they can simply be ignored. */ \
    if (COLLECT_ENTRIES)                                                \
      {                                                                 \
        /* Otherwise, clear the popped entries, so they can be       */ \
        /* collected as garbage if not referenced elsewhere.         */ \
        T##__leaf__ *const entries =                                    \
          ENTRIES_MALLOC (sizeof (T##__leaf__));                        \
        *entries = *vec_->tail;                                         \
        memset (&entries->a[new_tail_length], 0,                        \
                ((((size_t) 1) << (BITS)) - new_tail_length)            \
                * sizeof entries->a[0]);                                \
        vec_->tail = entries;                                           \
      }                                                                 \
  }                                                                     \
                                                                        \
  MODIFIER T                                                            \
  NAME (T vec, size_t count)                                            \
  {                                                                     \
    assert ((vec == (void *) 0) ?                                       \
            (count == 0) : (count <= vec->length));                     \
                                                                        \
    T##__vector__ *vec_;                                                \
                                                                        \
    if (count == 0)                                                     \
      /* No change. */                                                  \
      vec_ = (T##__vector__ *) vec;                                     \
    else if (count == vec->length)                                      \
      /* The new vector is empty; our representation for */             \
      /* an empty vector is a NULL pointer.              */             \
      vec_ = (T##__vector__ *) 0;                                       \
    else                                                                \
      {                                                                 \
        /* Make a new root. */                                          \
        vec_ = POINTERS_MALLOC (sizeof (T##__vector__));                \
        *vec_ = *vec;                                                   \
        vec_->length -= count;                                          \
                                                                        \
        const size_t tail_length = T##__tail_length__ (vec);            \
        const size_t new_tail_length = T##__tail_length__ (vec_);       \
        if (count < tail_length)                                        \
          /* Merely shorten the tail. */                                \
          NAME##__collect_entries__ (vec_, new_tail_length);            \
        else                                                            \
          {                                                             \
            const size_t mask = (((size_t) 1) << (BITS)) - 1;           \
            const size_t new_tail_offset =                              \
              vec_->length - new_tail_length;                           \
                                                                        \
            /* Find the new tail. */                                    \
            void *p = vec->node;                                        \
            for (size_t s = vec->shift; s != 0; s -= (BITS))            \
              {                                                         \
                T##__internal__ *const q = p;                           \
                p = q->a[(new_tail_offset >> s) & mask];                \
              }                                                         \
            vec_->tail = p;                                             \
            NAME##__collect_entries__ (vec_, new_tail_length);          \
                                                                        \
            if (new_tail_offset == 0)                                   \
              {                                                         \
                /* Delete the trie. We now have only a tail. */         \
                vec_->shift = 0;                                        \
                vec_->node = (void *) 0;                                \
              }                                                         \
            else                                                        \
              {                                                         \
                /* The index of the last entry in the new trie. */      \
                const size_t i = new_tail_offset - 1;                   \
                                                                        \
                /* Possibly reduce trie height. */                      \
                while (vec_->shift != 0                                 \
                       && ((i >> vec_->shift) & mask) == 0)             \
                  {                                                     \
                    vec_->node =                                        \
                      ((T##__internal__ *) vec_->node)->a[0];           \
                    vec_->shift -= (BITS);                              \
                  }                                                     \
                                                                        \
                /* Make new internal nodes. */                          \
                void **parent = &vec_->node;                            \
                p = *parent;                                            \
                for (size_t s = vec_->shift; s != 0; s -= (BITS))       \
                  {                                                     \
                    T##__internal__ *const q =                          \
                      POINTERS_MALLOC (sizeof (T##__internal__));       \
                    const size_t index = (i >> s) & mask;               \
                    for (size_t j = 0; j <= index; j++)                 \
                      q->a[j] = ((T##__internal__ *) p)->a[j];          \
                    *parent = q;                                        \
                    parent = &q->a[index];                              \
                    p = *parent;                                        \
                  }                                                     \
              }                                                         \
          }                                                             \
      }                                                                 \
                                                                        \
    T##__check_structure__ (vec_);                                      \
                                                                        \
    return vec_;                                                        \
  }

#define DECLARE_HIHA_PERSISTENT_VECTOR_SLICE(MODIFIER, NAME,    \
                                             T, ENTRY_T, BITS)  \
  MODIFIER T NAME (T, size_t, size_t);
#define DEFINE_HIHA_PERSISTENT_VECTOR_SLICE(MODIFIER, NAME,             \
                                            T, ENTRY_T, BITS,           \
                                            REFS, PUSHES, POPS)         \
  MODIFIER T                                                            \
  NAME (T vec, size_t start, size_t end)                                \
  {                                                                     \
    assert (start <= end);                                              \
                                                                        \
    const size_t length =                                               \
      (vec == (T##__vector__ *) 0) ? 0 : vec->length;                   \
    assert (end <= length);                                             \
                                                                        \
    T vec_;                                                             \
    if (start == 0)                                                     \
      vec_ = POPS (vec, length - end);                                  \
    else                                                                \
      {                                                                 \
        const size_t buffer_size = ((size_t) 1) << (BITS);              \
        ENTRY_T buffer[buffer_size];                                    \
                                                                        \
        vec_ = (T) 0;                                                   \
        size_t i = start;                                               \
        while (i < end)                                                 \
          {                                                             \
            const size_t j = (end < i + buffer_size) ?                  \
              end :                                                     \
              ((i + buffer_size) & ~((((size_t) 1) << (BITS)) - 1));    \
            REFS (vec, i, j - i, buffer);                               \
            vec_ = PUSHES (vec_, j - i, buffer);                        \
            i = j;                                                      \
          }                                                             \
      }                                                                 \
    return vec_;                                                        \
  }

#define DECLARE_HIHA_PERSISTENT_VECTOR_APPEND(MODIFIER, NAME,   \
                                              T, ENTRY_T, BITS) \
  MODIFIER T NAME (T, T);
#define DEFINE_HIHA_PERSISTENT_VECTOR_APPEND(MODIFIER, NAME,            \
                                             T, ENTRY_T, BITS,          \
                                             REFS, PUSHES)              \
  MODIFIER T                                                            \
  NAME (T v1, T v2)                                                     \
  {                                                                     \
    const size_t buffer_size = ((size_t) 1) << (BITS);                  \
    ENTRY_T buffer[buffer_size];                                        \
                                                                        \
    if (v2 != (T##__vector__ *) 0)                                      \
      {                                                                 \
        const size_t end = v2->length;                                  \
        size_t i = 0;                                                   \
        while (i < end)                                                 \
          {                                                             \
            const size_t j =                                            \
              (end < i + buffer_size) ? end : (i + buffer_size);        \
            REFS (v2, i, j - i, buffer);                                \
            v1 = PUSHES (v1, j - i, buffer);                            \
            i = j;                                                      \
          }                                                             \
      }                                                                 \
    return v1;                                                          \
  }

#define DECLARE_HIHA_PERSISTENT_VECTOR_MERGE(MODIFIER, NAME,            \
                                             T, ENTRY_T, BITS)          \
  MODIFIER T NAME (T, T,                                                \
                   int (*) (const ENTRY_T *, const ENTRY_T *, void *),  \
                   void *);
#define DEFINE_HIHA_PERSISTENT_VECTOR_MERGE(MODIFIER, NAME,             \
                                            T, ENTRY_T, BITS,           \
                                            PVECT_PTR, PVECT_NEXT,      \
                                            PUSHES)                     \
  MODIFIER T                                                            \
  NAME (T v1, T v2,                                                     \
        int (*cmp) (const ENTRY_T *, const ENTRY_T *, void *),          \
        void *data)                                                     \
  {                                                                     \
    T vec_;                                                             \
    if (v1 == (T) 0)                                                    \
      vec_ = v2;                                                        \
    else if (v2 == (T) 0)                                               \
      vec_ = v1;                                                        \
    else                                                                \
      {                                                                 \
        const size_t buffer_size = ((size_t) 1) << (BITS);              \
        ENTRY_T buffer[buffer_size];                                    \
                                                                        \
        vec_ = (T) 0;                                                   \
        size_t i1 = 0;                                                  \
        size_t i2 = 0;                                                  \
        const ENTRY_T *p1 = PVECT_PTR (v1, i1);                         \
        const ENTRY_T *p2 = PVECT_PTR (v2, i2);                         \
        size_t j = 0;                                                   \
        while (p1 != NULL && p2 != NULL)                                \
          {                                                             \
            const int c = cmp (p1, p2, data);                           \
            if (c <= 0)                                                 \
              {                                                         \
                buffer[j] = *p1;                                        \
                p1 = PVECT_NEXT (v1, i1, p1);                           \
                i1 += 1;                                                \
              }                                                         \
            else                                                        \
              {                                                         \
                buffer[j] = *p2;                                        \
                p2 = PVECT_NEXT (v2, i2, p2);                           \
                i2 += 1;                                                \
              }                                                         \
            j += 1;                                                     \
            if (j == buffer_size)                                       \
              {                                                         \
                vec_ = PUSHES (vec_, buffer_size, buffer);              \
                j = 0;                                                  \
              }                                                         \
          }                                                             \
        while (p1 != NULL)                                              \
          {                                                             \
            buffer[j] = *p1;                                            \
            j += 1;                                                     \
            if (j == buffer_size)                                       \
              {                                                         \
                vec_ = PUSHES (vec_, buffer_size, buffer);              \
                j = 0;                                                  \
              }                                                         \
            p1 = PVECT_NEXT (v1, i1, p1);                               \
            i1 += 1;                                                    \
          }                                                             \
        while (p2 != NULL)                                              \
          {                                                             \
            buffer[j] = *p2;                                            \
            j += 1;                                                     \
            if (j == buffer_size)                                       \
              {                                                         \
                vec_ = PUSHES (vec_, buffer_size, buffer);              \
                j = 0;                                                  \
              }                                                         \
            p2 = PVECT_NEXT (v2, i2, p2);                               \
            i2 += 1;                                                    \
          }                                                             \
        vec_ = PUSHES (vec_, j, buffer);                                \
      }                                                                 \
    return vec_;                                                        \
  }

/*--------------------------------------------------------------------*/

/*
  Effort-savers that do a whole lot at once.
*/

#define DECLARE_HIHA_PERSISTENT_VECTOR_DATATYPE(MODIFIER, TNAME,        \
                                                ENTRY_T, BITS)          \
  DECLARE_HIHA_PERSISTENT_VECTOR_T (MODIFIER, TNAME##_t, ENTRY_T,       \
                                    BITS);                              \
  DECLARE_HIHA_PERSISTENT_VECTOR_LENGTH (MODIFIER, TNAME##_length,      \
                                         TNAME##_t);                    \
  DECLARE_HIHA_PERSISTENT_VECTOR_REF (MODIFIER, TNAME##_ref,            \
                                      TNAME##_t, ENTRY_T, BITS);        \
  DECLARE_HIHA_PERSISTENT_VECTOR_PTR (MODIFIER, TNAME##_ptr,            \
                                      TNAME##_t, ENTRY_T, BITS);        \
  DECLARE_HIHA_PERSISTENT_VECTOR_NEXT (MODIFIER, TNAME##_next,          \
                                       TNAME##_t, ENTRY_T, BITS);       \
  DECLARE_HIHA_PERSISTENT_VECTOR_PREV (MODIFIER, TNAME##_prev,          \
                                       TNAME##_t, ENTRY_T, BITS);       \
  DECLARE_HIHA_PERSISTENT_VECTOR_REFS (MODIFIER, TNAME##_refs,          \
                                       TNAME##_t, ENTRY_T, BITS);       \
  DECLARE_HIHA_PERSISTENT_VECTOR_SET (MODIFIER, TNAME##_set,            \
                                      TNAME##_t, ENTRY_T, BITS);        \
  DECLARE_HIHA_PERSISTENT_VECTOR_SETS (MODIFIER, TNAME##_sets,          \
                                       TNAME##_t, ENTRY_T, BITS);       \
  DECLARE_HIHA_PERSISTENT_VECTOR_PUSH (MODIFIER, TNAME##_push,          \
                                       TNAME##_t, ENTRY_T, BITS);       \
  DECLARE_HIHA_PERSISTENT_VECTOR_PUSHES (MODIFIER, TNAME##_pushes,      \
                                         TNAME##_t, ENTRY_T, BITS);     \
  DECLARE_HIHA_PERSISTENT_VECTOR_POP (MODIFIER, TNAME##_pop,            \
                                      TNAME##_t, BITS);                 \
  DECLARE_HIHA_PERSISTENT_VECTOR_POPS (MODIFIER, TNAME##_pops,          \
                                       TNAME##_t, BITS);                \
  DECLARE_HIHA_PERSISTENT_VECTOR_SLICE (MODIFIER, TNAME##_slice,        \
                                        TNAME##_t, ENTRY_T, BITS);      \
  DECLARE_HIHA_PERSISTENT_VECTOR_APPEND (MODIFIER, TNAME##_append,      \
                                         TNAME##_t, ENTRY_T, BITS);     \
  DECLARE_HIHA_PERSISTENT_VECTOR_MERGE (MODIFIER, TNAME##_merge,        \
                                        TNAME##_t, ENTRY_T, BITS)

/*

  POINTERS_MALLOC() must zero out the memory it allocates.
 
  ENTRIES_MALLOC() has no such requirement. It may be allocated as
  unchecked memory, if it will contain no pointers.

  If ENTRY_T needs garbage collection you may also want to set
  COLLECT_ENTRIES true.

*/
#define DEFINE_HIHA_PERSISTENT_VECTOR_DATATYPE_GENERAL(MODIFIER,        \
                                                       TNAME,           \
                                                       ENTRY_T, BITS,   \
                                                       POINTERS_MALLOC, \
                                                       ENTRIES_MALLOC,  \
                                                       COLLECT_ENTRIES) \
  DEFINE_HIHA_PERSISTENT_VECTOR_T (MODIFIER, TNAME##_t, ENTRY_T, BITS)  \
  DEFINE_HIHA_PERSISTENT_VECTOR_LENGTH (MODIFIER, TNAME##_length,       \
                                        TNAME##_t);                     \
  DEFINE_HIHA_PERSISTENT_VECTOR_REF (MODIFIER, TNAME##_ref, TNAME##_t,  \
                                     ENTRY_T, BITS);                    \
  DEFINE_HIHA_PERSISTENT_VECTOR_PTR (MODIFIER, TNAME##_ptr, TNAME##_t,  \
                                     ENTRY_T, BITS);                    \
  DEFINE_HIHA_PERSISTENT_VECTOR_NEXT (MODIFIER, TNAME##_next,           \
                                      TNAME##_t, ENTRY_T, BITS);        \
  DEFINE_HIHA_PERSISTENT_VECTOR_PREV (MODIFIER, TNAME##_prev,           \
                                      TNAME##_t, ENTRY_T, BITS);        \
  DEFINE_HIHA_PERSISTENT_VECTOR_REFS(MODIFIER, TNAME##_refs,            \
                                     TNAME##_t, ENTRY_T, BITS,          \
                                     TNAME##_ptr, TNAME##_next);        \
  DEFINE_HIHA_PERSISTENT_VECTOR_SET (MODIFIER, TNAME##_set, TNAME##_t,  \
                                     ENTRY_T, BITS,                     \
                                     POINTERS_MALLOC, ENTRIES_MALLOC);  \
  DEFINE_HIHA_PERSISTENT_VECTOR_SETS (MODIFIER, TNAME##_sets,           \
                                      TNAME##_t, ENTRY_T, BITS,         \
                                      POINTERS_MALLOC,                  \
                                      ENTRIES_MALLOC);                  \
  DEFINE_HIHA_PERSISTENT_VECTOR_PUSH (MODIFIER, TNAME##_push,           \
                                      TNAME##_t, ENTRY_T, BITS,         \
                                      POINTERS_MALLOC,                  \
                                      ENTRIES_MALLOC);                  \
  DEFINE_HIHA_PERSISTENT_VECTOR_PUSHES (MODIFIER, TNAME##_pushes,       \
                                        TNAME##_t, ENTRY_T, BITS,       \
                                        POINTERS_MALLOC,                \
                                        ENTRIES_MALLOC);                \
  DEFINE_HIHA_PERSISTENT_VECTOR_POP (MODIFIER, TNAME##_pop,             \
                                     TNAME##_t, BITS,                   \
                                     POINTERS_MALLOC,                   \
                                     ENTRIES_MALLOC,                    \
                                     (COLLECT_ENTRIES));                \
  DEFINE_HIHA_PERSISTENT_VECTOR_POPS (MODIFIER, TNAME##_pops,           \
                                      TNAME##_t, BITS,                  \
                                      POINTERS_MALLOC,                  \
                                      ENTRIES_MALLOC,                   \
                                      (COLLECT_ENTRIES));               \
  DEFINE_HIHA_PERSISTENT_VECTOR_SLICE (MODIFIER, TNAME##_slice,         \
                                       TNAME##_t, ENTRY_T, BITS,        \
                                       TNAME##_refs, TNAME##_pushes,    \
                                       TNAME##_pops);                   \
  DEFINE_HIHA_PERSISTENT_VECTOR_APPEND (MODIFIER, TNAME##_append,       \
                                        TNAME##_t, ENTRY_T, BITS,       \
                                        TNAME##_refs, TNAME##_pushes);  \
  DEFINE_HIHA_PERSISTENT_VECTOR_MERGE (MODIFIER, TNAME##_merge,         \
                                        TNAME##_t, ENTRY_T, BITS,       \
                                       TNAME##_ptr, TNAME##_next,       \
                                       TNAME##_pushes)

#define DEFINE_HIHA_PERSISTENT_VECTOR_DATATYPE(MODIFIER, TNAME,         \
                                               ENTRY_T, BITS)           \
  DEFINE_HIHA_PERSISTENT_VECTOR_DATATYPE_GENERAL(MODIFIER, TNAME,       \
                                                 ENTRY_T, BITS,         \
                                                 HIHA_VECTOR_ZALLOCSZ,  \
                                                 HIHA_VECTOR_ZALLOCSZ,  \
                                                 true)

#define DEFINE_HIHA_PERSISTENT_VECTOR_DATATYPE_ATOMIC(MODIFIER, TNAME,  \
                                                      ENTRY_T, BITS)    \
  DEFINE_HIHA_PERSISTENT_VECTOR_DATATYPE_GENERAL(MODIFIER, TNAME,       \
                                                 ENTRY_T, BITS,         \
                                                 HIHA_VECTOR_ZALLOCSZ,  \
                                                 HIHA_VECTOR_ZALLOC_ATOMICSZ, \
                                                 false)

/*--------------------------------------------------------------------*/
/* Persistent vectors of const void pointers. */

DECLARE_HIHA_PERSISTENT_VECTOR_DATATYPE (, voidp_vector,
                                         const void *, 5);

/*--------------------------------------------------------------------*/

#endif /* __LIBHAHA__PERSISTENT_VECTOR_H__INCLUDED__ */

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
