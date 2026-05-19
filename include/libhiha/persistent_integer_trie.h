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

#ifndef __LIBHAHA__PERSISTENT_INTEGER_TRIE_H__INCLUDED__
#define __LIBHAHA__PERSISTENT_INTEGER_TRIE_H__INCLUDED__

/*

  Generic radix trees for unsigned integers as keys.

  The implementation below uses bit-indexing and thus does not require
  population counts nor arrays. The tree is binary. The price paid is
  that there will be more nodes in the tree.

*/

#include <stdlib.h>
#include <assert.h>

#ifndef HIHA_INT_TRIE_ALLOC
#include <xalloc.h>
#define HIHA_INT_TRIE_ALLOC(T) XMALLOC (T)
#endif

#define HIHA_INT_TRIE_NODE_DECL(NAME)           \
  typedef struct NAME                           \
  {                                             \
    bool is_leaf;                               \
  } *NAME##_t

#define HIHA_INT_TRIE_INTERNAL_DECL(NAME)       \
  typedef struct NAME##_internal                \
  {                                             \
    bool is_leaf;                               \
    struct NAME *left;                          \
    struct NAME *right;                         \
  } *NAME##_internal_t

#define HIHA_INT_TRIE_LEAF_DECL(NAME, UINTKEY, VALTYPE) \
  typedef struct NAME##_leaf                            \
  {                                                     \
    bool is_leaf;                                       \
    UINTKEY key;                                        \
    VALTYPE value;                                      \
  } *NAME##_leaf_t

#define HIHA_INT_TRIE_NODES_DECL(NAME, UINTKEY, VALTYPE)        \
  HIHA_INT_TRIE_NODE_DECL (NAME);                               \
  HIHA_INT_TRIE_INTERNAL_DECL (NAME);                           \
  HIHA_INT_TRIE_LEAF_DECL (NAME, UINTKEY, VALTYPE)

#define HIHA_INT_TRIE_MAKE_INTERNAL(NEW_NODE, NAME, LEFT, RIGHT)        \
  do                                                                    \
    {                                                                   \
      struct NAME##_internal *_NEW_ND__ =                               \
        HIHA_INT_TRIE_ALLOC (struct NAME##_internal);                   \
      _NEW_ND__->is_leaf = false;                                       \
      _NEW_ND__->left = (LEFT);                                         \
      _NEW_ND__->right = (RIGHT);                                       \
      NEW_NODE = (NAME##_t) _NEW_ND__;                                  \
    }                                                                   \
  while (0)

#define HIHA_INT_TRIE_MAKE_LEAF(NEW_NODE, NAME, KEY, VALUE)     \
  do                                                            \
    {                                                           \
      struct NAME##_leaf *_NEW_ND__ =                           \
        HIHA_INT_TRIE_ALLOC (struct NAME##_leaf);               \
      _NEW_ND__->is_leaf = true;                                \
      _NEW_ND__->key = (KEY);                                   \
      _NEW_ND__->value = (VALUE);                               \
      NEW_NODE = (NAME##_t) _NEW_ND__;                          \
    }                                                           \
  while (0)

#define HIHA_INT_TRIE_SEARCH(SOUGHT_NODE, NAME, UINTKEY, NODE, KEY)     \
  do                                                                    \
    {                                                                   \
      NAME##_t _SOUGHT_ND__ = (NODE);                                   \
      UINTKEY _KEY__ = (KEY);                                           \
      UINTKEY _MASK__ = 1;                                              \
      while (_SOUGHT_ND__ != NULL && !_SOUGHT_ND__->is_leaf)            \
        {                                                               \
          _SOUGHT_ND__ =                                                \
            (((_KEY__ & _MASK__) == 0)                                  \
             ? ((NAME##_internal_t) _SOUGHT_ND__)->left                 \
             : ((NAME##_internal_t) _SOUGHT_ND__)->right);              \
          _MASK__ = (_MASK__ << 1);                                     \
        }                                                               \
      if (_SOUGHT_ND__ != NULL)                                         \
        if (_KEY__ != ((NAME##_leaf_t) _SOUGHT_ND__)->key)              \
          _SOUGHT_ND__ = NULL;                                          \
      SOUGHT_NODE = (NAME##_leaf_t) _SOUGHT_ND__;                       \
    }                                                                   \
  while (0)

/* A walk of the leaf nodes, with callbacks. The order of the walk
   should be considered arbitrary. */
#define HIHA_INT_TRIE_WALK_DEFN(FUNC, NAME)                             \
  void                                                                  \
  FUNC (NAME##_t _Node, void (*_Do_something) (NAME##_leaf_t, void *),  \
        void *_Possibly_some_data)                                      \
  {                                                                     \
    if (_Node == NULL)                                                  \
      ; /* Do nothing. */                                               \
    else if (_Node->is_leaf)                                            \
      (_Do_something) ((NAME##_leaf_t) _Node, _Possibly_some_data);     \
    else                                                                \
      {                                                                 \
        (FUNC) (((NAME##_internal_t) _Node)->left,                      \
                (_Do_something), _Possibly_some_data);                  \
        (FUNC) (((NAME##_internal_t) _Node)->right,                     \
                (_Do_something), _Possibly_some_data);                  \
      }                                                                 \
  }

/* Search for a matching leaf node. Return NULL if none found. */
#define HIHA_INT_TRIE_SEARCH_DEFN(FUNC, NAME, UINTKEY)          \
  NAME##_leaf_t                                                 \
  FUNC (NAME##_t _Node, UINTKEY _Key)                           \
  {                                                             \
    NAME##_leaf_t _result;                                      \
    HIHA_INT_TRIE_SEARCH (_result, NAME, UINTKEY, _Node, _Key); \
    return _result;                                             \
  }

/* Insert a leaf node, nondestructively. */
#define HIHA_INT_TRIE_INSERT_DEFN(FUNC, NAME, UINTKEY, VALTYPE)         \
                                                                        \
  NAME##_t                                                              \
  FUNC##_86732d50_79a7_4c27_90a4_a295bf8822e2 (NAME##_t _Node,          \
                                               UINTKEY _Key,            \
                                               VALTYPE _Value,          \
                                               UINTKEY _Mask)           \
  {                                                                     \
    NAME##_t _result;                                                   \
    NAME##_t _nd;                                                       \
    if (_Node == NULL)                                                  \
      /* A new leaf. */                                                 \
      HIHA_INT_TRIE_MAKE_LEAF (_result, NAME, _Key, _Value);            \
    else if (_Node->is_leaf)                                            \
      {                                                                 \
        NAME##_leaf_t _Leaf = (NAME##_leaf_t) _Node;                    \
        if (_Key == _Leaf->key)                                         \
          /* An equal key, but a new value. */                          \
          HIHA_INT_TRIE_MAKE_LEAF (_result, NAME, _Key, _Value);        \
        else                                                            \
          {                                                             \
            /* Branch out. */                                           \
            bool _key_is_left = ((_Key & _Mask) == 0);                  \
            bool _leaf_is_left = ((_Leaf->key & _Mask) == 0);           \
            if (_key_is_left)                                           \
              {                                                         \
                if (_leaf_is_left)                                      \
                  {                                                     \
                    _nd =                                               \
                      ((FUNC##_86732d50_79a7_4c27_90a4_a295bf8822e2)    \
                       (_Node, _Key, _Value, _Mask << 1));              \
                    HIHA_INT_TRIE_MAKE_INTERNAL                         \
                      (_result, NAME, _nd, NULL);                       \
                  }                                                     \
                else                                                    \
                  {                                                     \
                    HIHA_INT_TRIE_MAKE_LEAF                             \
                      (_nd, NAME, _Key, _Value);                        \
                    HIHA_INT_TRIE_MAKE_INTERNAL                         \
                      (_result, NAME, _nd, _Node);                      \
                  }                                                     \
              }                                                         \
            else                                                        \
              {                                                         \
                if (_leaf_is_left)                                      \
                  {                                                     \
                    HIHA_INT_TRIE_MAKE_LEAF                             \
                      (_nd, NAME, _Key, _Value);                        \
                    HIHA_INT_TRIE_MAKE_INTERNAL                         \
                      (_result, NAME, _Node, _nd);                      \
                  }                                                     \
                else                                                    \
                  {                                                     \
                    _nd =                                               \
                      ((FUNC##_86732d50_79a7_4c27_90a4_a295bf8822e2)    \
                       (_Node, _Key, _Value, _Mask << 1));              \
                    HIHA_INT_TRIE_MAKE_INTERNAL                         \
                      (_result, NAME, NULL, _nd);                       \
                  }                                                     \
              }                                                         \
          }                                                             \
      }                                                                 \
    else                                                                \
      {                                                                 \
        /* Continue looking for the insertion point. */                 \
        NAME##_internal_t _Internal = (NAME##_internal_t) _Node;        \
        if ((_Key & _Mask) == 0)                                        \
          {                                                             \
            _nd = ((FUNC##_86732d50_79a7_4c27_90a4_a295bf8822e2)        \
                   (_Internal->left, _Key, _Value, _Mask << 1));        \
            HIHA_INT_TRIE_MAKE_INTERNAL                                 \
              (_result, NAME, _nd, _Internal->right);                   \
          }                                                             \
        else                                                            \
          {                                                             \
            _nd = ((FUNC##_86732d50_79a7_4c27_90a4_a295bf8822e2)        \
                   (_Internal->right, _Key, _Value, _Mask << 1));       \
            HIHA_INT_TRIE_MAKE_INTERNAL                                 \
              (_result, NAME, _Internal->left, _nd);                    \
          }                                                             \
      }                                                                 \
    return _result;                                                     \
  }                                                                     \
                                                                        \
  NAME##_t                                                              \
  FUNC (NAME##_t _Node, UINTKEY _Key, VALTYPE _Value)                   \
  {                                                                     \
    return ((FUNC##_86732d50_79a7_4c27_90a4_a295bf8822e2)               \
            (_Node, _Key, _Value, (UINTKEY) 1));                        \
  }

/* Delete a leaf node, nondestructively. */
#define HIHA_INT_TRIE_DELETE_DEFN(FUNC, NAME, UINTKEY)                  \
                                                                        \
  NAME##_t                                                              \
  FUNC##_86732d50_79a7_4c27_90a4_a295bf8822e2 (NAME##_t _Node,          \
                                               UINTKEY _Key,            \
                                               UINTKEY _Mask)           \
  {                                                                     \
    assert (_Node != NULL);                                             \
    NAME##_t _nd;                                                       \
    NAME##_t _result = NULL;                                            \
    if (!_Node->is_leaf)                                                \
      {                                                                 \
        NAME##_internal_t _Internal = (NAME##_internal_t) _Node;        \
        if ((_Key & _Mask) == 0)                                        \
          {                                                             \
            _nd = ((FUNC##_86732d50_79a7_4c27_90a4_a295bf8822e2)        \
                   (_Internal->left, _Key, _Mask << 1));                \
            if (_nd != NULL                                             \
                && _nd->is_leaf                                         \
                && _Internal->right == NULL)                            \
              _result = _nd;                                            \
            else if (_nd == NULL &&                                     \
                     _Internal->right != NULL                           \
                     && _Internal->right->is_leaf)                      \
              _result = _Internal->right;                               \
            else                                                        \
              HIHA_INT_TRIE_MAKE_INTERNAL                               \
                (_result, NAME, _nd, _Internal->right);                 \
          }                                                             \
        else                                                            \
          {                                                             \
            _nd = ((FUNC##_86732d50_79a7_4c27_90a4_a295bf8822e2)        \
                   (_Internal->right, _Key, _Mask << 1));               \
            if (_nd != NULL                                             \
                && _nd->is_leaf                                         \
                && _Internal->left == NULL)                             \
              _result = _nd;                                            \
            else if (_nd == NULL                                        \
                     && _Internal->left != NULL                         \
                     && _Internal->left->is_leaf)                       \
              _result = _Internal->left;                                \
            else                                                        \
              HIHA_INT_TRIE_MAKE_INTERNAL                               \
                (_result, NAME, _Internal->left, _nd);                  \
          }                                                             \
      }                                                                 \
    return _result;                                                     \
  }                                                                     \
                                                                        \
  NAME##_t                                                              \
  FUNC (NAME##_t _Node, UINTKEY _Key)                                   \
  {                                                                     \
    NAME##_t _result = _Node;                                           \
    NAME##_leaf_t _leaf;                                                \
    HIHA_INT_TRIE_SEARCH(_leaf, NAME, UINTKEY, _Node, _Key);            \
    if (_leaf != NULL)                                                  \
      _result =                                                         \
        (FUNC##_86732d50_79a7_4c27_90a4_a295bf8822e2) (_Node, _Key, 1); \
    return _result;                                                     \
  }

/* A simple test. This test requires GNU C nested functions, and on
   x86-64 most likely will require a trampoline on an executable
   stack, even if you use the compiler’s optimizer. A trampoline is
   needed for the nested callback function that calls other nested
   functions. */
#define HIHA_INT_TRIE_TEST_DEFN(FUNC, BUFSIZE)                          \
  void                                                                  \
  FUNC (void)                                                           \
  {                                                                     \
    static int buf[BUFSIZE];                                            \
                                                                        \
    void fisher_yates (void)                                            \
    {                                                                   \
      for (int i = 0; i != (BUFSIZE); i += 1)                           \
        buf[i] = i;                                                     \
      for (int i = (BUFSIZE) - 1; i != 0; i -= 1)                       \
        {                                                               \
          /* Dividing the big random integer is good enough. */         \
          int j = (int) (random () % ((long) i) + 1);                   \
                                                                        \
          int _tmp = buf[i];                                            \
          buf[i] = buf[j];                                              \
          buf[j] = _tmp;                                                \
        }                                                               \
    }                                                                   \
                                                                        \
    HIHA_INT_TRIE_NODES_DECL (uint_trie_int_node, unsigned int, int);   \
    HIHA_INT_TRIE_INSERT_DEFN (uint_trie_int_insert,                    \
                               uint_trie_int_node, unsigned int, int);  \
    HIHA_INT_TRIE_SEARCH_DEFN (uint_trie_int_search,                    \
                               uint_trie_int_node, unsigned int);       \
    HIHA_INT_TRIE_DELETE_DEFN (uint_trie_int_delete,                    \
                               uint_trie_int_node, unsigned int);       \
    HIHA_INT_TRIE_WALK_DEFN (uint_trie_int_walk, uint_trie_int_node);   \
                                                                        \
    uint_trie_int_node_t tree;                                          \
    uint_trie_int_node_t tree1;                                         \
    uint_trie_int_node_t tree2;                                         \
    uint_trie_int_node_t tree3;                                         \
    int i;                                                              \
    int j;                                                              \
                                                                        \
    /* Insert in random order. */                                       \
    fisher_yates ();                                                    \
    tree = NULL;                                                        \
    for (i = 0; i != (BUFSIZE); i += 1)                                 \
      tree = (uint_trie_int_insert) (tree, buf[i], -buf[i]);            \
                                                                        \
    /* Search in random order. */                                       \
    fisher_yates ();                                                    \
    for (i = 0; i != (BUFSIZE); i += 1)                                 \
      {                                                                 \
        uint_trie_int_node_leaf_t leaf =                                \
          (uint_trie_int_search) (tree, buf[i]);                        \
        assert (leaf != NULL);                                          \
        assert (leaf->key == buf[i]);                                   \
        assert (leaf->value == -buf[i]);                                \
      }                                                                 \
                                                                        \
    /* Delete in random order. */                                       \
    fisher_yates ();                                                    \
    for (i = 0; i != (BUFSIZE); i += 1)                                 \
      {                                                                 \
        tree = (uint_trie_int_delete) (tree, buf[i]);                   \
        for (j = 0; j != i + 1; j += 1)                                 \
          {                                                             \
            uint_trie_int_node_leaf_t leaf =                            \
              (uint_trie_int_search) (tree, buf[j]);                    \
            assert (leaf == NULL);                                      \
          }                                                             \
        for (j = i + 1; j != (BUFSIZE); j += 1)                         \
          {                                                             \
            uint_trie_int_node_leaf_t leaf =                            \
              (uint_trie_int_search) (tree, buf[j]);                    \
            assert (leaf != NULL);                                      \
            assert (leaf->key == buf[j]);                               \
            assert (leaf->value == -buf[j]);                            \
          }                                                             \
      }                                                                 \
    assert (tree == NULL);                                              \
                                                                        \
    /* Fill two trees. Walk through one and use the walk to delete */   \
    /* the contents of the other tree. Simultaneously use the walk */   \
    /* to build another tree.                                      */   \
    fisher_yates ();                                                    \
    tree1 = NULL;                                                       \
    tree2 = NULL;                                                       \
    tree3 = NULL;                                                       \
    for (i = 0; i != (BUFSIZE); i += 1)                                 \
      {                                                                 \
        tree1 = (uint_trie_int_insert) (tree1, buf[i], -buf[i]);        \
        tree2 = (uint_trie_int_insert) (tree2, buf[i], -buf[i]);        \
      }                                                                 \
    void delete_leaf (uint_trie_int_node_leaf_t leaf, void *data)       \
    {                                                                   \
      tree2 = (uint_trie_int_delete) (tree2, leaf->key);                \
      uint_trie_int_node_t tree3 = *((uint_trie_int_node_t *) data);    \
      tree3 = (uint_trie_int_insert) (tree3, leaf->key, leaf->value);   \
      *((uint_trie_int_node_t *) data) = tree3;                         \
    }                                                                   \
    (uint_trie_int_walk) (tree1, delete_leaf, &tree3);                  \
    assert (tree2 == NULL);                                             \
    for (i = 0; i != (BUFSIZE); i += 1)                                 \
      {                                                                 \
        assert (tree3 != NULL);                                         \
        tree3 = (uint_trie_int_delete) (tree3, buf[i]);                 \
      }                                                                 \
    assert (tree3 == NULL);                                             \
  }

#endif /* __LIBHAHA__PERSISTENT_INTEGER_TRIE_H__INCLUDED__ */

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
