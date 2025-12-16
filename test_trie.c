#include <stdio.h>

#include "libraries/mr4th/src/mr4th_base.h"
#include "libraries/mr4th/src/mr4th_base.c"

#include "source/steady_trie.h"

S32 main(void) {
  Arena *arena = arena_alloc_reserve(1024*1024, 0);
  Steady_Trie_Node *root = arena_push(arena, sizeof(Steady_Trie_Node));
  Steady_Trie_Node *child = arena_push(arena, sizeof(Steady_Trie_Node));

  root->occupied[17] = 1;
  root->slots[121] = child;
  child->occupied[64] = 1;

  steady_trie_print_trie(arena, root);

  return 0;
}
