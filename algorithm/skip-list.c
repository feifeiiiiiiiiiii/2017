#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>

#define MAX_LEVEL 32

typedef struct skip_list_node_s skip_list_node_t;
typedef struct skip_list_s skip_list_t;

struct skip_list_node_s {
  int key;
  int level;
  skip_list_node_t **forward;
};

struct skip_list_s {
  skip_list_node_t *head;
  int level;
};

skip_list_t *skip_list_init();
void skip_list_insert(skip_list_t *root, int key);
skip_list_node_t *skip_list_find(skip_list_t *root, int key);
void skip_list_remove(skip_list_t *root, int key);
void skip_list_free(skip_list_t *root);
static void skip_list_dump(skip_list_t *root);

static skip_list_node_t *skip_list_node_alloc(int key, int level) {
  skip_list_node_t *node = (skip_list_node_t *)malloc(sizeof(skip_list_node_t));
  node->key = key;
  node->forward = malloc(level * sizeof(skip_list_node_t));
  return node;
}

skip_list_t *skip_list_init() {
  skip_list_t *list = (skip_list_t *)malloc(sizeof(skip_list_t));
  list->level = 0;
  list->head = (skip_list_node_t *)malloc(sizeof(skip_list_node_t));
  list->head->forward = (skip_list_node_t **)malloc((MAX_LEVEL + 1)*sizeof(skip_list_node_t));
  list->head->key = INT_MAX;
  for(int i = 0; i <= MAX_LEVEL; ++i) {
    list->head->forward[i] = list->head;
  }
  return list;
}

static int rand_level() {
  int level = 0;

  while((rand()&1) == 1) {
	level++;
	if (level == MAX_LEVEL) {
	  break;
	}
  }
  return level;
}

void skip_list_insert(skip_list_t *root, int key) {
  skip_list_node_t *update[MAX_LEVEL + 1];
  skip_list_node_t *x = root->head;

  for(int i = root->level; i >= 0; i--) {
    while(x->forward[i]->key < key) x = x->forward[i];
    update[i] = x;
  }

  x = x->forward[0];
  if(key == x->key) {
    return;
  }
  int level = rand_level();
  if(level > root->level) {
    for(int i = root->level; i <= level; ++i) {
      update[i] = root->head;
    }
    root->level = level;
  }

  x = skip_list_node_alloc(key, level+1);
  for(int i = 0; i <= level; ++i) {
    x->forward[i] = update[i]->forward[i];
    update[i]->forward[i] = x;
  }
}

skip_list_node_t *skip_list_find(skip_list_t *root, int key) {
  skip_list_node_t *p = root->head;
  for(int i = root->level; i >= 0; i--) {
    while(p->forward[i]->key < key) {
      p = p->forward[i];
    }
  }
  if(p->forward[0]->key == key) return p;
  return NULL;
}

static void skip_list_node_free(skip_list_node_t *p) {
  free(p->forward);
  free(p);
}

void skip_list_remove(skip_list_t *root, int key) {
  skip_list_node_t *update[MAX_LEVEL+1];
  skip_list_node_t *p = root->head;

  for(int i = root->level; i >= 0; --i) {
    while(p->forward[i]->key < key) p = p->forward[i];
    update[i] = p;
  }
  p = p->forward[0];
  if(p->key == key) {
    for(int i = 0; i <= root->level; ++i) {
      if(update[i]->forward[i] != p) {
        break;
      }
      update[i]->forward[i] = p->forward[i];
    }
    skip_list_node_free(p);
    while (root->level > 0 && root->head->forward[root->level] == root->head) {
      root->level--;
    }
  }
}

static void skip_list_dump(skip_list_t *root) {
  skip_list_node_t *x = root->head;
  while(x && x->forward[0] != root->head) {
    printf("%d ", x->forward[0]->key);
    x = x->forward[0];
  }
}

void skip_list_free(skip_list_t *root) {
  skip_list_node_t *x = root->head->forward[0];
  while(x != root->head) {
    skip_list_node_t *next_node = x->forward[0];
    skip_list_node_free(x);
    x = next_node;
  }
  free(x->forward);
  free(x);
  free(root);
}

int main() {
  skip_list_t *root = skip_list_init();
  skip_list_insert(root, 10);
  skip_list_insert(root, 25);
  skip_list_insert(root, 30);
  skip_list_insert(root, 20);
  skip_list_insert(root, 50);
  assert(skip_list_find(root, 50) != NULL);
  assert(skip_list_find(root, -50) == NULL);
  skip_list_remove(root, 25);
  skip_list_dump(root);
  skip_list_free(root);
}

