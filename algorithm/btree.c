#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define MAX_NODE_KEY 2
#define TRUE 1
#define FALSE 0

typedef struct btree_node_s btree_node_t;

struct btree_node_s {
  int leaf;
  int n;
  int keys[MAX_NODE_KEY * 2 + 1];
  btree_node_t* childs[MAX_NODE_KEY * 2 + 1];
};

btree_node_t *btree_create();
static btree_node_t *btree_alloc_node();
btree_node_t *btree_search(btree_node_t *p, int key);
btree_node_t *btree_insert(btree_node_t **root, int key);
btree_node_t *btree_copy_node(btree_node_t *p);
void btree_split_child(btree_node_t **p, int i);
void btree_insert_nonfull(btree_node_t *p, int key);
void print_btree(btree_node_t *p);

static btree_node_t *btree_alloc_node() {
  btree_node_t *node = (btree_node_t *)malloc(sizeof(btree_node_t));
  node->leaf = TRUE;
  node->n = 0;
  return node;
}

btree_node_t *btree_create() {
  btree_node_t *node = btree_alloc_node();
  assert(node != NULL);
  return node;
}

btree_node_t *btree_search(btree_node_t *p, int key) {
  if(p == NULL) return NULL;

  int i = 0;
  while(i <= p->n && key > p->keys[i]) i = i + 1;

  if(i <= p->n && key == p->keys[i]) return p;
  else if(p->leaf) {
    return NULL;
  } else {
    // need disk-read
    return btree_search(p->childs[i], key);
  }
}

void btree_split_child(btree_node_t **p, int i) {
  btree_node_t *x = *p;
  btree_node_t *z = btree_alloc_node();
  btree_node_t *y = x->childs[i];

  z->leaf = y->leaf;
  z->n = MAX_NODE_KEY - 1;

  for(int j = 1; j < MAX_NODE_KEY; ++j) {
    z->keys[j] = y->keys[j+MAX_NODE_KEY];
  }

  if(y->leaf != TRUE) {
    for(int j = 1; j <= MAX_NODE_KEY; ++j) {
      z->childs[j] = y->childs[j+MAX_NODE_KEY];
    }
  }
  y->n = MAX_NODE_KEY - 1;
  for(int j = x->n + 1; j >= i + 1; j--) {
    x->childs[j+1] = x->childs[j];
  }

  x->childs[i+1] = z;

  for(int j = x->n; j >= i; --j) {
    x->keys[j+1] = x->keys[j];
  }
  x->keys[i] = y->keys[MAX_NODE_KEY];
  x->n = x->n + 1;
}

void btree_insert_nonfull(btree_node_t *p, int key) {
  int i = p->n;

  if(p->leaf) {
    while(i >= 1 && key < p->keys[i]) {
      p->keys[i+1] = p->keys[i];
      i = i - 1;
    }
    p->keys[i+1] = key;
    p->n = p->n + 1;
  } else {
    while(i >= 1 && key < p->keys[i]) {
      i = i - 1;
    }
    i = i + 1;
    if(p->childs[i]->n == 2 * MAX_NODE_KEY - 1) {
      btree_split_child(&p, i);
      if(key > p->keys[i]) i = i +1;
    }
    btree_insert_nonfull(p->childs[i], key);
  }
}

btree_node_t *btree_copy_node(btree_node_t *p) {
  btree_node_t *node = btree_alloc_node();
  node->n = p->n;
  node->leaf = p->leaf;

  for(int i = 1; i <= node->n; ++i) {
    node->keys[i] = p->keys[i];
  }

  for(int i = 1; i <= node->n+1; ++i) {
    node->childs[i] = p->childs[i];
  }
  return node;
}

btree_node_t *btree_insert(btree_node_t **root, int key) {
  if((*root)->n == 2 * MAX_NODE_KEY - 1) {
    btree_node_t *old_root = btree_copy_node(*root);
    btree_node_t *node = btree_alloc_node();
    node->leaf = FALSE;
    node->n = 0;
    node->childs[1] = old_root;
    *root = node;

    btree_split_child(&node, 1);
    btree_insert_nonfull(node, key);
  } else {
    btree_insert_nonfull(*root, key);
  }
  return NULL;
}

void print_btree(btree_node_t *p) {
  for(int i = 1; i <= p->n; ++i) {
    if(p->leaf == FALSE) {
      print_btree(p->childs[i]);
    }
    printf("%d ", p->keys[i]);
  }
  if(p->leaf == FALSE) {
    print_btree(p->childs[p->n + 1]);
  }
}

int main() {
  btree_node_t *root = btree_create();
  btree_insert(&root, 1);
  btree_insert(&root, 2);
  btree_insert(&root, 3);
  btree_insert(&root, 4);
  btree_insert(&root, 6);
  btree_insert(&root, 5);
  btree_insert(&root, 7);
  btree_insert(&root, 8);
  btree_insert(&root, 9);
  print_btree(root);
  return 0;
}
