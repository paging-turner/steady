#include <stdio.h>

#include "libraries/mr4th/src/mr4th_base.h"
#include "libraries/mr4th/src/mr4th_base.c"

#include "source/steady_trie.h"

static B32
steady_trie_ensure_key_has_occupation(Steady_Trie_Node *root, Steady_Trie_Key key, B32 occupation) {
  B32 exists = steady_trie_search(root, key);
  B32 errors = 0;

  if (exists != occupation) {
    printf("[ Error ] Expected key %llu to have occupation %d but it has occupation %d\n", (U64)key, occupation, exists);
    errors = 1;
  }

  return errors;
}

static void steady_trie_run_tests(void) {
  Arena *arena = arena_alloc_reserve(1024*1024, 0);
  Steady_Trie_Node *root = arena_push(arena, sizeof(Steady_Trie_Node));
  U32 error_count = 0;

  // TODO: turn the following "tests" into actual tests that automatically detect errors. That way, we can try various configurations of trie settings to see if they all work.
  U64 a = 189;
  U64 b = 242;
  U64 c = 42387468;
#if Steady_Trie_Key_Bits == 64
  U64 d = 12370169555311111083LLU;
#elif Steady_Trie_Key_Bits == 32
  U64 d = 1237;
#endif
  U64 e = 123701695;
  U64 f = 9287349786368457;

  U64 keys_to_add[] = {a, b, c, d};
  U64 keys_to_delete[] = {b, d};

  printf("Inserting keys...\n");
  for (U32 i = 0; i < ArrayCount(keys_to_add); ++i) {
    steady_trie_insert(arena, root, keys_to_add[i]);
  }

  printf("Checking key occupancy...\n");
  error_count += steady_trie_ensure_key_has_occupation(root, a, 1);
  error_count += steady_trie_ensure_key_has_occupation(root, b, 1);
  error_count += steady_trie_ensure_key_has_occupation(root, c, 1);
  error_count += steady_trie_ensure_key_has_occupation(root, d, 1);
  error_count += steady_trie_ensure_key_has_occupation(root, e, 0);
  error_count += steady_trie_ensure_key_has_occupation(root, f, 0);

  printf("Checking iterated keys...\n");
  U32 match_count = 0;
  Steady_Trie_Iterate(iter_name, arena, root) {
    B32 match = 0;
    for (U32 i = 0; i < ArrayCount(keys_to_add); ++i) {
      if (iter_name->key == keys_to_add[i]) {
        match_count += 1;
        match = 1;
        break;
      }
    }
    if (!match) {
      printf("[ Error ] Found unmatched key while iterating, %llu\n", (U64)iter_name->key);
      error_count += 1;
    }
  }
  if (match_count != ArrayCount(keys_to_add)) {
    printf("[ Error ] Expected to match %lu keys while iterating but only matched %d\n", ArrayCount(keys_to_add), match_count);
    error_count += 1;
  }

  printf("Deleting keys...\n");
  for (U32 i = 0; i < ArrayCount(keys_to_delete); ++i) {
    steady_trie_delete(root, keys_to_delete[i]);
  }

  printf("Checking iterated keys...\n");
  match_count = 0;
  Steady_Trie_Iterate(iter_name, arena, root) {
    B32 to_add_match = 0;
    for (U32 i = 0; i < ArrayCount(keys_to_add); ++i) {
      if (iter_name->key == keys_to_add[i]) {
        match_count += 1;
        to_add_match = 1;
        break;
      }
    }
    if (!to_add_match) {
      printf("[ Error ] Found unmatched key while iterating, %llu\n", (U64)iter_name->key);
      error_count += 1;
    }
    B32 to_delete_match = 0;
    for (U32 i = 0; i < ArrayCount(keys_to_delete); ++i) {
      if (iter_name->key == keys_to_delete[i]) {
        to_delete_match = 1;
        break;
      }
    }
    if (to_delete_match) {
      printf("[ Error ] Found matched key that should have been deleted, %llu\n", (U64)iter_name->key);
      error_count += 1;
    }
  }
  if (match_count != ArrayCount(keys_to_add) - ArrayCount(keys_to_delete)) {
    printf("[ Error ] Expected to match %lu keys while iterating but only matched %d\n", ArrayCount(keys_to_add), match_count);
    error_count += 1;
  }


  printf("==================\n");
  printf("== End of tests ==\n");
  printf("==================\n");
  if (error_count) {
    printf("%d FAILURES.\n", error_count);
  }
  else {
    printf("Tests PASSED.\n");
  }
}


static void steady_trie_print_struct_sizes(void) {
  printf("Steady_Trie_Key_Bits %d\n", Steady_Trie_Key_Bits);
  printf("Steady_Trie_Slot_Bits %d\n", Steady_Trie_Slot_Bits);
  printf("Steady_Trie_Slot_Count %d\n", Steady_Trie_Slot_Count);
  printf("Steady_Trie_Max_Depth %d\n", Steady_Trie_Max_Depth);
  printf("\n");
  printf("sizeof(Steady_Trie_Node) %zu\n", sizeof(Steady_Trie_Node));
  printf("sizeof(Steady_Trie_Stack_Node) %zu\n", sizeof(Steady_Trie_Stack_Node));
  printf("sizeof(Steady_Trie_Iterator) %zu\n", sizeof(Steady_Trie_Iterator));
}



S32 main(void) {
  steady_trie_run_tests();
  printf("\n\n");
  steady_trie_print_struct_sizes();

  return 0;
}
