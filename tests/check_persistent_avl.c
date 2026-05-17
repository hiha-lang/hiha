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
#include <gc/gc.h>
#include <libhiha/libhiha.h>

HIHA_VISIBLE const char version_etc_copyright[] =
  "Copyright %s %d Barry Schwartz";

#define BUFSIZE 10000

static int buf[BUFSIZE];

static void
fisher_yates (void)
{
  for (int i = 0; i != (BUFSIZE); i += 1)
    buf[i] = i;
  for (int i = (BUFSIZE) - 1; i != 0; i -= 1)
    {
      /* Dividing the big random integer is good enough. */
      int j = (int) (random () % ((long) i) + 1);

      int _tmp = buf[i];
      buf[i] = buf[j];
      buf[j] = _tmp;
    }
}

static int
int_cmp (const int *a, const int *b)
{
  return (*a < *b) ? -1 : ((*b < *a) ? 1 : 0);
}

HIHA_AVL_NODE_DECL (avl_int_node, int);
HIHA_AVL_VERIFY_DEFN (avl_int_verify, avl_int_node);
HIHA_AVL_INORDER_DEFN (avl_int_inorder, avl_int_node);
HIHA_AVL_SEARCH_DEFN (avl_int_search, avl_int_node, int);
HIHA_AVL_INSERT_DEFN (avl_int_insert, avl_int_node, int);
HIHA_AVL_DELETE_DEFN (avl_int_delete, avl_int_node, int);

static void
test_backwards (avl_int_node_t node, void *data)
{
  int *p = (int *) data;
  *p += -1;
  assert (node->data == *p);
}

static void
test_forwards (avl_int_node_t node, void *data)
{
  int *p = (int *) data;
  assert (node->data == *p);
  *p += 1;
}

static void
avl_test (void)
{
  avl_int_node_t tree;
  int i;
  int j;

  /* Insert in random order. */
  fisher_yates ();
  tree = NULL;
  assert (0 <= (avl_int_verify) (tree));
  for (i = 0; i != (BUFSIZE); i += 1)
    {
      tree = (avl_int_insert) (tree, buf[i], (int_cmp));
      assert (0 <= (avl_int_verify) (tree));
    }

  /* Search in random order. */
  fisher_yates ();
  for (i = 0; i != (BUFSIZE); i += 1)
    assert ((avl_int_search) (tree, buf[i], (int_cmp)) != NULL);

  /* In-order walk backwards. */
  i = BUFSIZE;
  (avl_int_inorder) (tree, -1, test_backwards, &i);

  /* In-order walk forwards. */
  i = 0;
  (avl_int_inorder) (tree, 1, test_forwards, &i);

  /* Delete in random order. */
  fisher_yates ();
  for (i = 0; i != (BUFSIZE); i += 1)
    {
      tree = (avl_int_delete) (tree, buf[i], (int_cmp));
      assert (0 <= (avl_int_verify) (tree));
      for (j = 0; j != i + 1; j += 1)
        assert ((avl_int_search) (tree, buf[j], (int_cmp)) == NULL);
      for (j = i + 1; j != (BUFSIZE); j += 1)
        assert ((avl_int_search) (tree, buf[j], (int_cmp)) != NULL);
    }
  assert (tree == NULL);

  /* Insert in random order. */
  fisher_yates ();
  tree = NULL;
  assert (0 <= (avl_int_verify) (tree));
  for (i = 0; i != (BUFSIZE); i += 1)
    {
      tree = (avl_int_insert) (tree, buf[i], (int_cmp));
      assert (0 <= (avl_int_verify) (tree));
    }

  /* Delete in ascending order. */
  fisher_yates ();
  for (i = 0; i != (BUFSIZE); i += 1)
    {
      tree = (avl_int_delete) (tree, i, (int_cmp));
      assert (0 <= (avl_int_verify) (tree));
      for (j = 0; j != i + 1; j += 1)
        assert ((avl_int_search) (tree, j, (int_cmp)) == NULL);
      for (j = i + 1; j != (BUFSIZE); j += 1)
        assert ((avl_int_search) (tree, j, (int_cmp)) != NULL);
    }
  assert (tree == NULL);
}

int
main (void)
{
  GC_INIT ();
  avl_test ();
  return 0;
}
