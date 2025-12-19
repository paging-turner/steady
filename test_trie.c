#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "libraries/mr4th/src/mr4th_base.h"
#include "libraries/mr4th/src/mr4th_base.c"

#include "libraries/pcg/pcg_basic.h"
#include "libraries/pcg/pcg_basic.c"

#if 1
# define Steady_Trie_Use_Key_Value_Pair 1
# define Steady_Trie_Value_Type U64
# define Steady_Trie_Default_Value 42LLU
# define Steady_Trie_Values_Equal(x, y)  ((x) == (y))
#endif
#include "source/steady_trie.h"

#define Steady_Trie_Test_Arena_Size  (50*1024*1024)



static pcg32_random_t Steady_Rng;

// NOTE: The comments here are from example code of the PCG library
static void SetPcgSeed(pcg32_random_t *Rng, B32 Nondeterministic, U32 Rounds)
{
  if (Nondeterministic)
  {
    // Seed with external entropy -- the time and some program addresses
    // (which will actually be somewhat random on most modern systems).
    // A better solution, entropy_getbytes, using /dev/random, is provided
    // in the full library.

    pcg32_srandom_r(Rng, time(NULL) ^ (intptr_t)&printf,
                    (intptr_t)&Rounds);
  }
  else
  {
    // Seed with a fixed constant
    pcg32_srandom_r(Rng, 42u, 54u);
  }
}




static B32
steady_trie_ensure_key_has_occupation(Steady_Trie_Node *root, Steady_Trie_Key key, B32 occupation) {
  B32 errors = 0;
#if Steady_Trie_Use_Key_Value_Pair
  Steady_Trie_Value_Type *value = steady_trie_search(root, key);

  if (occupation && !value) {
    printf("[ Error ] Null value at key %llu\n", (U64)key);
    errors = 1;
  }
  else if (occupation && !Steady_Trie_Values_Equal(*value, Steady_Trie_Default_Value)) {
    printf("[ Error ] Mismatched value at key %llu, expecting %llu but got %llu\n", (U64)key, Steady_Trie_Default_Value, *value);
    errors = 1;
  }
  else if (!occupation && value) {
    printf("[ Error ] Found a value at key %llu, but was not expecting a value.\n", (U64)key);
    errors = 1;
  }
#else
  B32 exists = steady_trie_search(root, key);

  if (exists != occupation) {
    printf("[ Error ] Expected key %llu to have occupation %d but it has occupation %d\n", (U64)key, occupation, exists);
    errors = 1;
  }
#endif

  return errors;
}

static U32 steady_trie_run_tests(void) {
  Arena *arena = arena_alloc_reserve(Steady_Trie_Test_Arena_Size, 0);
  Steady_Trie_Node *root = arena_push(arena, sizeof(Steady_Trie_Node));
  U32 error_count = 0;

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

  printf("=================\n");
  printf("== Begin tests ==\n");
  printf("=================\n");

  printf("Inserting keys...\n");
  for (U32 i = 0; i < ArrayCount(keys_to_add); ++i) {
#if Steady_Trie_Use_Key_Value_Pair
    steady_trie_set(arena, root, keys_to_add[i], Steady_Trie_Default_Value);
#else
    steady_trie_insert(arena, root, keys_to_add[i]);
#endif
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
    printf("[ Error ] Expected to match %lu keys while iterating but only matched %d\n", ArrayCount(keys_to_add)-ArrayCount(keys_to_delete), match_count);
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

  return error_count;
}


static void steady_trie_print_key_efficiency(Arena *arena, U64 key_count) {
#if Steady_Trie_Use_Key_Value_Pair
  U64 raw_key_size = key_count * (sizeof(Steady_Trie_Key) + sizeof(Steady_Trie_Value_Type));
#else
  U64 raw_key_size = key_count * sizeof(Steady_Trie_Key);
#endif
  U64 arena_pos = arena->chunk_pos;
  printf("Arena position %llu\n", arena_pos);
  printf("Raw key size %llu\n", raw_key_size);
  printf("Percent memory used for keys %.2f%%\n", (F32)raw_key_size/(F32)arena_pos);
}

static void steady_trie_sequential_keys_test(void) {
  Arena *arena = arena_alloc_reserve(Steady_Trie_Test_Arena_Size, 0);
  Steady_Trie_Node *root = arena_push(arena, sizeof(Steady_Trie_Node));

  U64 key_count = 5000;

  printf("Inserted %llu sequential keys...\n", key_count);
  for (U32 i = 0; i < key_count; ++i) {
#if Steady_Trie_Use_Key_Value_Pair
    steady_trie_set(arena, root, i, Steady_Trie_Default_Value);
#else
    steady_trie_insert(arena, root, i);
#endif
  }

  steady_trie_print_key_efficiency(arena, key_count);
}


static void steady_trie_random_32bit_keys_test(void) {
  Arena *arena = arena_alloc_reserve(Steady_Trie_Test_Arena_Size, 0);
  SetPcgSeed(&Steady_Rng, 1, 1);
  Steady_Trie_Node *root = arena_push(arena, sizeof(Steady_Trie_Node));

  U64 key_count = 5000;

  printf("Inserted %llu random keys...\n", key_count);
  for (U32 i = 0; i < key_count; ++i) {
    U64 random_key = pcg32_boundedrand_r(&Steady_Rng, max_U32);
#if Steady_Trie_Use_Key_Value_Pair
    steady_trie_set(arena, root, random_key, Steady_Trie_Default_Value);
#else
    steady_trie_insert(arena, root, random_key);
#endif
  }

  steady_trie_print_key_efficiency(arena, key_count);
}


static void steady_trie_malloc_pointers_test(void) {
  Arena *arena = arena_alloc_reserve(Steady_Trie_Test_Arena_Size, 0);
  Steady_Trie_Node *root = arena_push(arena, sizeof(Steady_Trie_Node));

  U64 key_count = 5000;
  U32 malloc_size = 16;

  printf("Inserted %llu malloc'ed pointers...\n", key_count);
  for (U32 i = 0; i < key_count; ++i) {
    U8 *pointer = malloc(malloc_size);
#if Steady_Trie_Use_Key_Value_Pair
    steady_trie_set(arena, root, (U64)pointer, Steady_Trie_Default_Value);
#else
    steady_trie_insert(arena, root, (U64)pointer);
#endif
  }

  steady_trie_print_key_efficiency(arena, key_count);
}


static void steady_trie_print_struct_sizes(void) {
  printf("==================\n");
  printf("== Struct Sizes ==\n");
  printf("==================\n");

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
  U32 error_count = steady_trie_run_tests();

  if (error_count == 0) {
    printf("\n\n");
    steady_trie_sequential_keys_test();
    printf("\n\n");
    steady_trie_random_32bit_keys_test();
    printf("\n\n");
#if Steady_Trie_Key_Bits == 64
    steady_trie_malloc_pointers_test();
    printf("\n\n");
#endif
  }

  steady_trie_print_struct_sizes();

  return 0;
}
