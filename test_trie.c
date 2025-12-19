#include <stdio.h>

#include "libraries/mr4th/src/mr4th_base.h"
#include "libraries/mr4th/src/mr4th_base.c"

#include "source/steady_trie.h"

S32 main(void) {
  Arena *arena = arena_alloc_reserve(1024*1024, 0);
  Steady_Trie_Node *root = arena_push(arena, sizeof(Steady_Trie_Node));

  U64 a = 189;
  U64 b = 242;
  U64 c = 42387468;
  U64 d = 12370169555311111083LLU;
  U64 e = 123701695;
  U64 f = 9287349786368457;

  printf("inserting %llu\n", a);
  steady_trie_insert(arena, root, a);
  printf("inserting %llu\n", b);
  steady_trie_insert(arena, root, b);
  printf("inserting %llu\n", c);
  steady_trie_insert(arena, root, c);
  printf("inserting %llu\n", d);
  steady_trie_insert(arena, root, d);

  printf("\n");
  printf("search %d %llu\n", steady_trie_search(root, a), a);
  printf("search %d %llu\n", steady_trie_search(root, b), b);
  printf("search %d %llu\n", steady_trie_search(root, c), c);
  printf("search %d %llu\n", steady_trie_search(root, d), d);
  printf("search %d %llu\n", steady_trie_search(root, e), e);
  printf("search %d %llu\n", steady_trie_search(root, f), f);

  printf("\niterating...\n");
  steady_trie_print_trie(arena, root);

  printf("\n");
  printf("deleting %llu\n", b);
  steady_trie_delete(root, b);
  printf("deleting %llu\n", d);
  steady_trie_delete(root, d);
  printf("search %d %llu\n", steady_trie_search(root, b), b);
  printf("search %d %llu\n", steady_trie_search(root, d), d);

  printf("\niterating...\n");
  steady_trie_print_trie(arena, root);

  return 0;
}
