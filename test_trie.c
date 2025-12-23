// TODO: Remove Trie_A specific test functions and more to steady_trie.h for now...




#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "libraries/mr4th/src/mr4th_base.h"
#include "libraries/mr4th/src/mr4th_base.c"

#include "libraries/pcg/pcg_basic.h"
#include "libraries/pcg/pcg_basic.c"

#define global_variable static


#define Steady_Trie(ident)  Trie_A_##ident
#define steady_trie(ident)  trie_a_##ident
#define Steady_Trie_Key_Bits 64
#define Steady_Trie_Slot_Bits 2
#define Steady_Trie_Root_Is_Lowest_Significant_Byte 1
#define Steady_Trie_Use_Key_Value_Pair 0
#include "source/steady_trie.h"


#define Steady_Trie(ident)  Trie_B_##ident
#define steady_trie(ident)  trie_b_##ident
#define Steady_Trie_Key_Bits 64
#define Steady_Trie_Slot_Bits 4
#define Steady_Trie_Root_Is_Lowest_Significant_Byte 0
#define Steady_Trie_Use_Key_Value_Pair 0
#include "source/steady_trie.h"

#define Steady_Trie(ident)  Trie_C_##ident
#define steady_trie(ident)  trie_c_##ident
#define Steady_Trie_Key_Bits 64
#define Steady_Trie_Slot_Bits 4
#define Steady_Trie_Root_Is_Lowest_Significant_Byte 0
#define Steady_Trie_Use_Key_Value_Pair 1
typedef struct foo {
  U64 bar;
} foo;
# define Steady_Trie_Value_Type foo
# define Steady_Trie_Default_Value (foo){0}
static B32 steady_trie(values_equal)(foo v1, foo v2){ return v1.bar == v2.bar; }
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




#if 0
static B32 steady_trie_ensure_key_has_occupation(
  Arena *arena,
  Trie_A_Trie *trie,
  Trie_A_Key key,
  B32 occupation
  ) {
  B32 errors = 0;

  if (trie->settings.use_key_value_pair) {
    Trie_A_Edit_Result search_result = trie_a_search(arena, trie, key);
    Trie_A_Value_Type *value = search_result.value;

    if (occupation && !value) {
      printf("[ Error ] Null value at key %llu\n", (U64)key);
      errors = 1;
    }
    // TODO: Check that values are equal.
    else if (occupation && !steady_trie_values_equal(*value, Steady_Trie_Default_Value)) {
      printf("[ Error ] Mismatched value at key %llu, expecting %llu but got %llu\n", (U64)key, Steady_Trie_Default_Value, *value);
      errors = 1;
    }
    else if (!occupation && value) {
      printf("[ Error ] Found a value at key %llu, but was not expecting a value.\n", (U64)key);
      errors = 1;
    }
  }
  else {
    Trie_A_Edit_Result search_result = trie_a_search(arena, trie, key);

    if (search_result.found != occupation) {
      printf("[ Error ] Expected key %llu to have occupation %d but it has occupation %d\n", (U64)key, occupation, search_result.found);
      errors = 1;
    }
  }

  return errors;
}
#endif


static void steady_trie_print_key_efficiency(Arena *arena, U64 key_count) {
#if Steady_Trie_Use_Key_Value_Pair
  U64 raw_key_size = key_count * (sizeof(Trie_A_Key) + sizeof(Steady_Trie_Value_Type));
#else
  U64 raw_key_size = key_count * sizeof(Trie_A_Key);
#endif
  U64 arena_pos = arena->chunk_pos;
  printf("Arena position %llu\n", arena_pos);
  printf("Raw key size %llu\n", raw_key_size);
  printf("Percent memory used for keys %.2f%%\n", (F32)raw_key_size/(F32)arena_pos);
}


static void steady_trie_sequential_keys_test(void) {
  Arena *arena = arena_alloc_reserve(Steady_Trie_Test_Arena_Size, 0);
  Trie_A_Trie *trie = trie_a_create_trie(arena);

  U64 key_count = 5000;

  printf("Inserted %llu sequential keys...\n", key_count);
  for (U32 i = 0; i < key_count; ++i) {
#if Steady_Trie_Use_Key_Value_Pair
    steady_trie_set(arena, trie, i, Steady_Trie_Default_Value);
#else
    trie_a_insert(arena, trie, i);
#endif
  }

  steady_trie_print_key_efficiency(arena, key_count);
}


static void steady_trie_random_32bit_keys_test(void) {
  Arena *arena = arena_alloc_reserve(Steady_Trie_Test_Arena_Size, 0);
  SetPcgSeed(&Steady_Rng, 1, 1);
  Trie_A_Trie *trie = trie_a_create_trie(arena);

  U64 key_count = 5000;

  printf("Inserted %llu random keys...\n", key_count);
  for (U32 i = 0; i < key_count; ++i) {
    U64 random_key = pcg32_boundedrand_r(&Steady_Rng, max_U32);
#if Steady_Trie_Use_Key_Value_Pair
    steady_trie_set(arena, trie, random_key, Steady_Trie_Default_Value);
#else
    trie_a_insert(arena, trie, random_key);
#endif
  }

  steady_trie_print_key_efficiency(arena, key_count);
}


static void steady_trie_malloc_pointers_test(void) {
  Arena *arena = arena_alloc_reserve(Steady_Trie_Test_Arena_Size, 0);
  Trie_A_Trie *trie = trie_a_create_trie(arena);

  U64 key_count = 5000;
  U32 malloc_size = 16;

  printf("Inserted %llu malloc'ed pointers...\n", key_count);
  for (U32 i = 0; i < key_count; ++i) {
    U8 *pointer = malloc(malloc_size);
#if Steady_Trie_Use_Key_Value_Pair
    steady_trie_set(arena, trie, (U64)pointer, Steady_Trie_Default_Value);
#else
    trie_a_insert(arena, trie, (U64)pointer);
#endif
  }

  steady_trie_print_key_efficiency(arena, key_count);
}


static void steady_trie_print_struct_sizes(Trie_A_Trie *trie) {
  printf("==================\n");
  printf("== Struct Sizes ==\n");
  printf("==================\n");

  printf("Steady_Trie_Key_Bits   %d\n", trie->settings.key_bits  );
  printf("Steady_Trie_Slot_Bits  %d\n", trie->settings.slot_bits );
  printf("Steady_Trie_Slot_Count %d\n", trie->settings.slot_count);
  printf("Steady_Trie_Max_Depth  %d\n", trie->settings.max_depth );
  printf("\n");
  printf("sizeof(Trie_A_Node      ) %zu\n", sizeof(Trie_A_Node      ));
  printf("sizeof(Trie_A_Stack_Node) %zu\n", sizeof(Trie_A_Stack_Node));
  printf("sizeof(Trie_A_Iterator  ) %zu\n", sizeof(Trie_A_Iterator  ));
}




S32 main(void) {
  Arena *test_arena = arena_alloc_reserve(Steady_Trie_Test_Arena_Size, 0);

  U32 error_count = trie_a_run_tests(test_arena);
  arena_pop_to(test_arena, 0);

  error_count    += trie_b_run_tests(test_arena);
  arena_pop_to(test_arena, 0);

  error_count    += trie_c_run_tests(test_arena);
  arena_pop_to(test_arena, 0);

#if 0
  printf("\n\n");
  steady_trie_sequential_keys_test();
  printf("\n\n");
  steady_trie_random_32bit_keys_test();
  printf("\n\n");
# if Steady_Trie_Key_Bits == 64
  steady_trie_malloc_pointers_test();
  printf("\n\n");
# endif
#endif

  /* Arena *debug_arena = arena_alloc_reserve(Steady_Trie_Test_Arena_Size, 0); */
  /* Trie_A_Trie *debug_trie = trie_a_create_trie(debug_arena); */
  /* steady_trie_print_struct_sizes(debug_trie); */

  return 0;
}
