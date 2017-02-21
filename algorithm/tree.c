#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct tree_node_s tree_node_t;

struct tree_node_s {
  int key;
  tree_node_t *left;
  tree_node_t *right;
  tree_node_t *parent;
};

tree_node_t *tree_search(tree_node_t *p, int key);
tree_node_t *new_node(int key);
tree_node_t *tree_successor(tree_node_t *p);
tree_node_t *tree_minimum(tree_node_t *p);
tree_node_t *tree_maximum(tree_node_t *p);
tree_node_t *tree_precursor(tree_node_t *p);
void tree_inorder_walk(tree_node_t *p);
void tree_free(tree_node_t *p);

tree_node_t *new_node(int key) {
  tree_node_t *node = (tree_node_t *)malloc(sizeof(tree_node_t));

  if(node == NULL) return NULL;

  node->key = key;
  node->left = NULL;
  node->right = NULL;
  node->parent = NULL;

  return node;
}

tree_node_t *tree_search(tree_node_t *p, int key) {
  if(p == NULL || p->key == key) {
    return p;
  }
  if(key < p->key) {
    return tree_search(p->left, key);
  }
  return tree_search(p->right, key);
}

tree_node_t *tree_minimum(tree_node_t *p) {
  while(p && p->left != NULL) {
    p = p->left;
  }
  return p;
}

tree_node_t *tree_maximum(tree_node_t *p) {
  while(p && p->right != NULL) {
    p = p->right;
  }
  return p;
}

tree_node_t *tree_successor(tree_node_t *p) {
  if(p->right != NULL) return tree_minimum(p->right);
  tree_node_t *y = p->parent;
  while(y != NULL && p == y->right) {
    p = y;
    y = y->parent;
  }
  return y;
}

tree_node_t *tree_precursor(tree_node_t *p) {
  if(p->left != NULL) return tree_maximum(p->left);
  tree_node_t *y = p->parent;
  while(y != NULL && p == y->left) {
    p = y;
    y = y->parent;
  }
  return y;
}


// 高质量C/C++编程指南
// 如果函数的参数是一个指针，不要指望用该指针去申请动态内存
// 编译器总是要为函数的每个参数制作临时副本，指针参数tree的副本是 _tree，
// 编译器使 _tree = tree。如果函数体内的程序修改了_tree的内容，就导致参数tree的内容作相应的修改。这就是指针可以用作输出参数的原因
int insert(tree_node_t **root, int key) {
  tree_node_t *node = new_node(key);
  if(node == NULL) return -1;

  tree_node_t *y = NULL;
  tree_node_t *x = *root;

  while(x != NULL) {
    y = x;
    if(node->key < x->key) x = x->left;
    else x = x->right;
  }

  if(y == NULL) {
    *root = node;
  } else if(node->key < y->key) {
    y->left = node;
    node->parent = y;
  } else {
    y->right = node;
    node->parent = y;
  }
  return 0;
}

void tree_inorder_walk(tree_node_t *p) {
  if(p == NULL) return;
  tree_inorder_walk(p->left);
  printf("%d ", p->key);
  tree_inorder_walk(p->right);
}

void tree_free(tree_node_t *p) {
  if(p == NULL) return;
  tree_free(p->left);
  tree_free(p->right);
  free(p);
}

int main() {
  tree_node_t *root = NULL;

  insert(&root, 2);
  insert(&root, 3);
  insert(&root, 1);
  insert(&root, 6);
  insert(&root, 0);
  insert(&root, 10);
  insert(&root, -10);
  insert(&root, 2);
  insert(&root, 5);

  tree_node_t *p = tree_search(root, 3);
  if(p != NULL) {
    printf("find result = %d\n", p->key);
    p = tree_precursor(p);
    if(p != NULL) {
      printf("find precursor = %d\n", p->key);
    }
  } else {
    printf("not found\n");
  }

  printf("print order: ");
  tree_inorder_walk(root);
  printf("\n");

  tree_free(root);
}

