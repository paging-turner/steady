#include <stdio.h>
#include <time.h>

#define MR4TH_ASSERTS 1
#include "libraries/mr4th/src/mr4th_base.h"
#include "libraries/mr4th/src/mr4th_base.c"

#include "libraries/pcg/pcg_basic.h"
#include "libraries/pcg/pcg_basic.c"

#define Steady_Arena  Arena
#define steady_arena_push_size(arena, size)  arena_push(arena, size)

#define Steady_Queue_Value_Type int

#define Steady_Queue_Slow_Value_Type  Steady_Queue_Value_Type
#include "steady_list_slow.h"

#define Steady_Queue_Copy_Value_Type  Steady_Queue_Value_Type
#include "steady_list_copy.h"

static pcg32_random_t Steady_Rng;



typedef enum {
  Steady_Queue_Action_Kind_Push,
  Steady_Queue_Action_Kind_PushFront,
  Steady_Queue_Action_Kind_Pop,
  Steady_Queue_Action_Kind_Delete,
  Steady_Queue_Action_Kind__Count,
} Steady_Queue_Action_Kind;

typedef struct {
  Steady_Queue_Action_Kind kind;
  Steady_Queue_Value_Type value;
  U64 id;
} Steady_Queue_Action;

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



static Steady_Queue_Action *steady_setup_queue_actions(Arena *arena, U32 action_count) {
  Steady_Queue_Action *actions = arena_push(arena, action_count * sizeof(Steady_Queue_Action));
  U32 opl_max_node_id = 0;

  if (actions) {
    for (U32 i = 0; i < action_count; ++i) {
      Steady_Queue_Action *action = actions + i;
      U32 action_kind = pcg32_boundedrand_r(&Steady_Rng, Steady_Queue_Action_Kind__Count);
      Assert(action_kind >= 0 && action_kind < Steady_Queue_Action_Kind__Count);
      action->kind = action_kind;

      // don't delete if it's the first action
      if (opl_max_node_id == 0 && action_kind == Steady_Queue_Action_Kind_Delete) {
        action_kind = Steady_Queue_Action_Kind_Push;
        action->kind = Steady_Queue_Action_Kind_Push;
      }

      if (action_kind == Steady_Queue_Action_Kind_Push ||
          action_kind == Steady_Queue_Action_Kind_PushFront) {
        action->value = pcg32_boundedrand_r(&Steady_Rng, 999999);
        opl_max_node_id += 1;
      } else if (action_kind == Steady_Queue_Action_Kind_Delete) {
        action->id = pcg32_boundedrand_r(&Steady_Rng, opl_max_node_id);
      }
    }
  }

  return actions;
}

static void steady_debug_print_queue_actions(Steady_Queue_Action *actions, U32 action_count) {
  // debug print out actions
  printf("Actions[%d]:\n", action_count);
  for (U32 i = 0; i < action_count; ++i) {
    Steady_Queue_Action *action = actions + i;
    switch (action->kind) {
    case Steady_Queue_Action_Kind_Push: {
      printf("  Push %d\n", action->value);
    } break;
    case Steady_Queue_Action_Kind_PushFront: {
      printf("PushFront %d\n", action->value);
    } break;
    case Steady_Queue_Action_Kind_Pop: {
      printf("Pop\n");
    } break;
    case Steady_Queue_Action_Kind_Delete: {
      printf("Delete %llu\n", action->id);
    } break;
    }
  }
}



int main(void) {
  Arena *slow_arena = arena_alloc_reserve(10*1024*1024, 0);
  Arena *copy_arena = arena_alloc_reserve(10*1024*1024, 0);
  Arena *action_arena = arena_alloc_reserve(1*1024*1024, 0);

  SetPcgSeed(&Steady_Rng, 1, 1);


  U32 action_count = 16;
  Steady_Queue_Action *actions = steady_setup_queue_actions(action_arena, action_count);

  steady_debug_print_queue_actions(actions, action_count);

#if 0
  Steady_Queue_Slow queue = (Steady_Queue_Slow){0};
  Steady_Queue_Slow_Node node_a = (Steady_Queue_Slow_Node){0};
  node_a.value = 32;
  Steady_Queue_Slow_Node node_b = (Steady_Queue_Slow_Node){0};
  node_b.value = 123;
  Steady_Queue_Slow_Node node_c = (Steady_Queue_Slow_Node){0};
  node_c.value = 999;

  steady_queue_slow_push(slow_arena, &queue, &node_a);
  steady_queue_slow_push(slow_arena, &queue, &node_b);
  steady_queue_slow_push_front(slow_arena, &queue, &node_c);
  steady_queue_slow_delete(slow_arena, &queue, node_a.id);
  steady_queue_slow_pop(slow_arena, &queue);

  S32 version_index = 0;
  for (Steady_Queue_Slow_Version *v = queue.first_version; v != 0; v = v->next_version) {
    printf("Version %2d: ", version_index++);
    for (Steady_Queue_Slow_Node *n = v->first; n != 0; n = n->next) {
      printf("%d ", n->value);
    }
    printf("\n");
  }
#elif 1
  Steady_Queue_Copy queue = (Steady_Queue_Copy){0};
  Steady_Queue_Copy_Node node_a = (Steady_Queue_Copy_Node){0};
  node_a.value = 32;
  Steady_Queue_Copy_Node node_b = (Steady_Queue_Copy_Node){0};
  node_b.value = 123;
  Steady_Queue_Copy_Node node_c = (Steady_Queue_Copy_Node){0};
  node_c.value = 999;

  steady_queue_copy_push(slow_arena, &queue, &node_a);
  steady_queue_copy_push(slow_arena, &queue, &node_b);
  steady_queue_copy_push(slow_arena, &queue, &node_c);

  S32 version_index = 0;
  for (Steady_Queue_Copy_Version *v = queue.first_version; v != 0; v = v->next_version) {
    printf("Version %2d: ", version_index++);
    for (Steady_Queue_Copy_Node *n = v->first; n != 0; n = n->next) {
      printf("%d ", n->value);
    }
    printf("\n");
  }
#endif

  return 0;
}
