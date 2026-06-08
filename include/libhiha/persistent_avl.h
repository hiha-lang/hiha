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

#ifndef __LIBHAHA__PERSISTENT_AVL_H__INCLUDED__
#define __LIBHAHA__PERSISTENT_AVL_H__INCLUDED__

#include <stdlib.h>
#include <assert.h>

#ifndef HIHA_AVL_ALLOC
#include <xalloc.h>
#define HIHA_AVL_ALLOC(T) XMALLOC (T)
#endif

#define HIHA_AVL_HEIGHT_T unsigned short int
#define HIHA_AVL_NODE_DECL(NAME, T)             \
  typedef struct NAME                           \
  {                                             \
    T data;                                     \
    HIHA_AVL_HEIGHT_T height;                   \
    struct NAME *left;                          \
    struct NAME *right;                         \
  } *NAME##_t;

#define HIHA_AVL_NODE_BALANCE(NAME, NODE)                               \
  (((NODE) == NULL) ? ((int) 0) :                                       \
   (((int) ((NODE)->left == NULL) ? 0 : (NODE)->left->height)           \
    - ((int) ((NODE)->right == NULL) ? 0 : (NODE)->right->height)))

#define HIHA_AVL_MAKE_NODE(NEW_NODE, NAME, DATA, LEFT, RIGHT)           \
  do                                                                    \
    {                                                                   \
      struct NAME *_node__ = HIHA_AVL_ALLOC (struct NAME);              \
      _node__->data = (DATA);                                           \
      _node__->left = (LEFT);                                           \
      _node__->right = (RIGHT);                                         \
      const HIHA_AVL_HEIGHT_T _Lheight =                                \
        (_node__->left == NULL) ? 0 : _node__->left->height;            \
      const HIHA_AVL_HEIGHT_T _Rheight =                                \
        (_node__->right == NULL) ? 0 : _node__->right->height;          \
      _node__->height = /* (the greater of the two heights) + 1 */      \
        ((_Lheight < _Rheight) ? _Rheight : _Lheight) + 1;              \
      NEW_NODE = _node__;                                               \
    }                                                                   \
  while (0)

/* Nondestructive rotation. */
#define HIHA_AVL_ROTATE_LEFT(NEW_NODE, NAME, NODE)      \
  do                                                    \
    {                                                   \
      struct NAME *_nd__ = (NODE);                      \
      struct NAME *_nd_R__ = _nd__->right;              \
      struct NAME *_new__;                              \
      HIHA_AVL_MAKE_NODE (_new__, NAME, _nd__->data,    \
                          _nd__->left, _nd_R__->left);  \
      HIHA_AVL_MAKE_NODE (_new__, NAME, _nd_R__->data,  \
                          _new__, _nd_R__->right);      \
      NEW_NODE = _new__;                                \
    }                                                   \
  while (0)

/* Nondestructive rotation. */
#define HIHA_AVL_ROTATE_RIGHT(NEW_NODE, NAME, NODE)             \
  do                                                            \
    {                                                           \
      struct NAME *_nd__ = (NODE);                              \
      struct NAME *_nd_L__ = _nd__->left;                       \
      struct NAME *_new__;                                      \
      HIHA_AVL_MAKE_NODE (_new__, NAME, _nd__->data,            \
                          _nd_L__->right, _nd__->right);        \
      HIHA_AVL_MAKE_NODE (_new__, NAME, _nd_L__->data,          \
                          _nd_L__->left, _new__);               \
      NEW_NODE = _new__;                                        \
    }                                                           \
  while (0)

/* Verify that a tree is balanced. Return the height if the tree is
   balanced, or -1 if it is not. */
#define HIHA_AVL_VERIFY_DEFN(FUNC, NAME)                \
  int                                                   \
  FUNC (struct NAME *_Node)                             \
  {                                                     \
    int result = -1;                                    \
    if (_Node == NULL)                                  \
      result = 0;                                       \
    else                                                \
      {                                                 \
        int height_L = (FUNC) (_Node->left);            \
        if (height_L != -1)                             \
          {                                             \
            int height_R = (FUNC) (_Node->right);       \
            if (height_R != -1)                         \
              {                                         \
                int diff = height_L - height_R;         \
                if (diff == -1 || diff == 0)            \
                  result = height_R + 1;                \
                else if (diff == 1)                     \
                  result = height_L + 1;                \
              }                                         \
          }                                             \
      }                                                 \
    return result;                                      \
  }

/* An in-order walk with callbacks. */
#define HIHA_AVL_INORDER_DEFN(FUNC, NAME)                       \
  void                                                          \
  FUNC (struct NAME *_Node, int _Direction,                     \
        void (*_Do_something) (struct NAME *, void *),          \
        void *_Possibly_some_data)                              \
  {                                                             \
    assert (_Direction == -1 || _Direction == 1);               \
    if (_Node != NULL)                                          \
      {                                                         \
        if (_Direction == -1)                                   \
          {                                                     \
            (FUNC) (_Node->right, _Direction, (_Do_something),  \
                    _Possibly_some_data);                       \
            (_Do_something) (_Node, _Possibly_some_data);       \
            (FUNC) (_Node->left, _Direction, (_Do_something),   \
                    _Possibly_some_data);                       \
          }                                                     \
        else                                                    \
          {                                                     \
            (FUNC) (_Node->left, _Direction, (_Do_something),   \
                    _Possibly_some_data);                       \
            (_Do_something) (_Node, _Possibly_some_data);       \
            (FUNC) (_Node->right, _Direction, (_Do_something),  \
                    _Possibly_some_data);                       \
          }                                                     \
      }                                                         \
  }

/* Search for a matching node. */
#define HIHA_AVL_SEARCH_DEFN(FUNC, NAME, T)                     \
  struct NAME *                                                 \
  FUNC (struct NAME *_Node, T _Data,                            \
        int (*_Compare) (const T *, const T *, void *),         \
        void *_Extra)                                           \
  {                                                             \
    struct NAME *_Result = NULL;                                \
    if (_Node != NULL)                                          \
      {                                                         \
        int _Cmp = (_Compare) (&_Data, &_Node->data, _Extra);   \
        if (_Cmp < 0)                                           \
          _Result =                                             \
            (FUNC) (_Node->left, _Data, (_Compare), _Extra);    \
        else if (_Cmp == 0)                                     \
          _Result = _Node;                                      \
        else                                                    \
          _Result =                                             \
            (FUNC) (_Node->right, _Data, (_Compare), _Extra);   \
      }                                                         \
    return _Result;                                             \
  }

/* Insert a node, nondestructively. */
#define HIHA_AVL_INSERT_DEFN(FUNC, NAME, T)                             \
  struct NAME *                                                         \
  FUNC (struct NAME *_Node, T _Data,                                    \
        int (*_Compare) (const T *, const T *, void *),                 \
        void *_Extra)                                                   \
  {                                                                     \
    struct NAME *_Result;                                               \
    if (_Node == NULL)                                                  \
      HIHA_AVL_MAKE_NODE (_Result, NAME, _Data, NULL, NULL);            \
    else                                                                \
      {                                                                 \
        int _Cmp = (_Compare) (&_Data, &_Node->data, _Extra);           \
        if (_Cmp == 0)                                                  \
          /* Same key, different value. */                              \
          HIHA_AVL_MAKE_NODE (_Result, NAME, _Data, _Node->left,        \
                              _Node->right);                            \
        else                                                            \
          {                                                             \
            if (_Cmp < 0)                                               \
              HIHA_AVL_MAKE_NODE (_Result, NAME, _Node->data,           \
                                  (FUNC) (_Node->left, _Data,           \
                                          (_Compare), _Extra),          \
                                  _Node->right);                        \
            else                                                        \
              HIHA_AVL_MAKE_NODE (_Result, NAME, _Node->data,           \
                                  _Node->left,                          \
                                  (FUNC) (_Node->right, _Data,          \
                                          (_Compare), _Extra));         \
                                                                        \
            /* Rebalance. */                                            \
            int _Bal = HIHA_AVL_NODE_BALANCE (NAME, _Result);           \
            if (_Bal < -1)                                              \
              {                                                         \
                _Cmp =                                                  \
                  (_Compare) (&_Data, &_Result->right->data, _Extra);   \
                if (_Cmp < 0)                                           \
                  {                                                     \
                    struct NAME *_nd;                                   \
                    HIHA_AVL_ROTATE_RIGHT (_nd, NAME, _Result->right);  \
                    HIHA_AVL_MAKE_NODE (_nd, NAME, _Result->data,       \
                                        _Result->left, _nd);            \
                    HIHA_AVL_ROTATE_LEFT (_Result, NAME, _nd);          \
                  }                                                     \
                else if (0 < _Cmp)                                      \
                  HIHA_AVL_ROTATE_LEFT (_Result, NAME, _Result);        \
              }                                                         \
            else if (1 < _Bal)                                          \
              {                                                         \
                _Cmp =                                                  \
                  (_Compare) (&_Data, &_Result->left->data, _Extra);    \
                if (_Cmp < 0)                                           \
                  HIHA_AVL_ROTATE_RIGHT (_Result, NAME, _Result);       \
                else if (0 < _Cmp)                                      \
                  {                                                     \
                    struct NAME *_nd;                                   \
                    HIHA_AVL_ROTATE_LEFT (_nd, NAME, _Result->left);    \
                    HIHA_AVL_MAKE_NODE (_nd, NAME, _Result->data,       \
                                        _nd, _Result->right);           \
                    HIHA_AVL_ROTATE_RIGHT (_Result, NAME, _nd);         \
                  }                                                     \
              }                                                         \
          }                                                             \
      }                                                                 \
    return _Result;                                                     \
  }

/* Delete a node, nondestructively. */
#define HIHA_AVL_DELETE_DEFN(FUNC, NAME, T)                             \
  struct NAME *                                                         \
  FUNC (struct NAME *_Node, T _Data,                                    \
        int (*_Compare) (const T *, const T *, void *),                 \
        void *_Extra)                                                   \
  {                                                                     \
    struct NAME *_Result = NULL;                                        \
    if (_Node != NULL)                                                  \
      {                                                                 \
        bool needs_rebalancing = false;                                 \
        int _Cmp = (_Compare) (&_Data, &_Node->data, _Extra);           \
        if (_Cmp < 0)                                                   \
          {                                                             \
            HIHA_AVL_MAKE_NODE (_Result, NAME, _Node->data,             \
                                (FUNC) (_Node->left, _Data,             \
                                        (_Compare), _Extra),            \
                                _Node->right);                          \
            needs_rebalancing = true;                                   \
          }                                                             \
        else if (0 < _Cmp)                                              \
          {                                                             \
            HIHA_AVL_MAKE_NODE (_Result, NAME, _Node->data,             \
                                _Node->left,                            \
                                (FUNC) (_Node->right, _Data,            \
                                        (_Compare), _Extra));           \
            needs_rebalancing = true;                                   \
          }                                                             \
        else                                                            \
          {                                                             \
            /* _Node is the node to be deleted.                    */   \
            if (_Node->left == NULL)                                    \
              /* _Node is a leaf or _Node->right is a leaf.        */   \
              /* The result will be NULL or a leaf.                */   \
              _Result = _Node->right;                                   \
            else if (_Node->right == NULL)                              \
              /* _Node->left is a leaf. The result will be a leaf. */   \
              _Result = _Node->left;                                    \
            else                                                        \
              {                                                         \
                /* Find the in-order successor to _Node.           */   \
                struct NAME *_succ = _Node->right;                      \
                while (_succ->left != NULL)                             \
                  _succ = _succ->left;                                  \
                                                                        \
                /* Move the successor to where _Node is.           */   \
                HIHA_AVL_MAKE_NODE (_Result, NAME, _succ->data,         \
                                    _Node->left,                        \
                                    (FUNC) (_Node->right, _succ->data,  \
                                            (_Compare), _Extra));       \
                needs_rebalancing = true;                               \
              }                                                         \
          }                                                             \
        if (_Result != NULL && needs_rebalancing)                       \
          {                                                             \
            int _Bal = HIHA_AVL_NODE_BALANCE (NAME, _Result);           \
            if (_Bal < -1)                                              \
              {                                                         \
                int _BalR =                                             \
                  HIHA_AVL_NODE_BALANCE (NAME, _Result->right);         \
                if (_BalR <= 0)                                         \
                  HIHA_AVL_ROTATE_LEFT (_Result, NAME, _Result);        \
                else                                                    \
                  {                                                     \
                    struct NAME *_nd;                                   \
                    HIHA_AVL_ROTATE_RIGHT (_nd, NAME, _Result->right);  \
                    HIHA_AVL_MAKE_NODE (_nd, NAME, _Result->data,       \
                                        _Result->left, _nd);            \
                    HIHA_AVL_ROTATE_LEFT (_Result, NAME, _nd);          \
                  }                                                     \
              }                                                         \
            else if (1 < _Bal)                                          \
              {                                                         \
                int _BalL =                                             \
                  HIHA_AVL_NODE_BALANCE (NAME, _Result->left);          \
                if (0 <= _BalL)                                         \
                  HIHA_AVL_ROTATE_RIGHT (_Result, NAME, _Result);       \
                else                                                    \
                  {                                                     \
                    struct NAME *_nd;                                   \
                    HIHA_AVL_ROTATE_LEFT (_nd, NAME, _Result->left);    \
                    HIHA_AVL_MAKE_NODE (_nd, NAME, _Result->data,       \
                                        _nd, _Result->right);           \
                    HIHA_AVL_ROTATE_RIGHT (_Result, NAME, _nd);         \
                  }                                                     \
              }                                                         \
          }                                                             \
      }                                                                 \
    return _Result;                                                     \
  }

/* A simple test. This test requires GNU C nested functions. */
#define HIHA_AVL_TEST_DEFN(FUNC, BUFSIZE)                               \
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
    int int_cmp (const int *a, const int *b)                            \
    {                                                                   \
      return (*a < *b) ? -1 : ((*b < *a) ? 1 : 0);                      \
    }                                                                   \
                                                                        \
    HIHA_AVL_NODE_DECL (avl_int_node, int);                             \
    HIHA_AVL_VERIFY_DEFN (avl_int_verify, avl_int_node);                \
    HIHA_AVL_INORDER_DEFN(avl_int_inorder, avl_int_node);               \
    HIHA_AVL_SEARCH_DEFN (avl_int_search, avl_int_node, int);           \
    HIHA_AVL_INSERT_DEFN (avl_int_insert, avl_int_node, int);           \
    HIHA_AVL_DELETE_DEFN (avl_int_delete, avl_int_node, int);           \
                                                                        \
    avl_int_node_t tree;                                                \
    int i;                                                              \
    int j;                                                              \
                                                                        \
    void test_backwards (avl_int_node_t node, void *data)               \
    {                                                                   \
      int *p = (int *) data;                                            \
      *p += -1;                                                         \
      assert (node->data == *p);                                        \
    }                                                                   \
                                                                        \
    void test_forwards (avl_int_node_t node, void *data)                \
    {                                                                   \
      int *p = (int *) data;                                            \
      assert (node->data == *p);                                        \
      *p += 1;                                                          \
    }                                                                   \
                                                                        \
    /* Insert in random order. */                                       \
    fisher_yates ();                                                    \
    tree = NULL;                                                        \
    assert (0 <= (avl_int_verify) (tree));                              \
    for (i = 0; i != (BUFSIZE); i += 1)                                 \
      {                                                                 \
        tree = (avl_int_insert) (tree, buf[i], (int_cmp));              \
        assert (0 <= (avl_int_verify) (tree));                          \
      }                                                                 \
                                                                        \
    /* Search in random order. */                                       \
    fisher_yates ();                                                    \
    for (i = 0; i != (BUFSIZE); i += 1)                                 \
      assert ((avl_int_search) (tree, buf[i], (int_cmp)) != NULL);      \
                                                                        \
    /* In-order walk backwards. */                                      \
    i = BUFSIZE;                                                        \
    (avl_int_inorder) (tree, -1, test_backwards, &i);                   \
                                                                        \
    /* In-order walk forwards. */                                       \
    i = 0;                                                              \
    (avl_int_inorder) (tree, 1, test_forwards, &i);                     \
                                                                        \
    /* Delete in random order. */                                       \
    fisher_yates ();                                                    \
    for (i = 0; i != (BUFSIZE); i += 1)                                 \
      {                                                                 \
        tree = (avl_int_delete) (tree, buf[i], (int_cmp));              \
        assert (0 <= (avl_int_verify) (tree));                          \
        for (j = 0; j != i + 1; j += 1)                                 \
          assert ((avl_int_search) (tree, buf[j], (int_cmp)) == NULL);  \
        for (j = i + 1; j != (BUFSIZE); j += 1)                         \
          assert ((avl_int_search) (tree, buf[j], (int_cmp)) != NULL);  \
      }                                                                 \
    assert (tree == NULL);                                              \
                                                                        \
    /* Insert in random order. */                                       \
    fisher_yates ();                                                    \
    tree = NULL;                                                        \
    assert (0 <= (avl_int_verify) (tree));                              \
    for (i = 0; i != (BUFSIZE); i += 1)                                 \
      {                                                                 \
        tree = (avl_int_insert) (tree, buf[i], (int_cmp));              \
        assert (0 <= (avl_int_verify) (tree));                          \
      }                                                                 \
                                                                        \
    /* Delete in ascending order. */                                    \
    fisher_yates ();                                                    \
    for (i = 0; i != (BUFSIZE); i += 1)                                 \
      {                                                                 \
        tree = (avl_int_delete) (tree, i, (int_cmp));                   \
        assert (0 <= (avl_int_verify) (tree));                          \
        for (j = 0; j != i + 1; j += 1)                                 \
          assert ((avl_int_search) (tree, j, (int_cmp)) == NULL);       \
        for (j = i + 1; j != (BUFSIZE); j += 1)                         \
          assert ((avl_int_search) (tree, j, (int_cmp)) != NULL);       \
      }                                                                 \
    assert (tree == NULL);                                              \
  }

#endif /* __LIBHAHA__PERSISTENT_AVL_H__INCLUDED__ */

/*
  local variables:
  mode: c
  coding: utf-8
  end:
*/
