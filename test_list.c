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



typedef enum {
  Steady_Queue_Implementation_Slow,
  Steady_Queue_Implementation_Copy,
  Steady_Queue_Implementation__Count,
} Steady_Queue_Implementation;



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

typedef struct {
  Steady_Queue_Action *actions;
  U32 count;
} Steady_Queue_Action_Buffer;


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

static void steady_debug_print_queue_actions(Steady_Queue_Action_Buffer *action_buffer) {
  // debug print out actions
  printf("Actions[%d]:\n", action_buffer->count);
  for (U32 i = 0; i < action_buffer->count; ++i) {
    Steady_Queue_Action *action = action_buffer->actions + i;

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



static void handle_queue_action_slow(Steady_Arena *arena, Steady_Queue_Slow *queue, Steady_Queue_Action *action) {
  switch (action->kind) {
  case Steady_Queue_Action_Kind_Push: {
    Steady_Queue_Slow_Node *node = arena_push(arena, sizeof(Steady_Queue_Slow_Node));
    node->value = action->value;
    steady_queue_slow_push(arena, queue, node);
  } break;
  case Steady_Queue_Action_Kind_PushFront: {
    Steady_Queue_Slow_Node *node = arena_push(arena, sizeof(Steady_Queue_Slow_Node));
    node->value = action->value;
    steady_queue_slow_push_front(arena, queue, node);
  } break;
  case Steady_Queue_Action_Kind_Pop: {
    steady_queue_slow_pop(arena, queue);
  } break;
  case Steady_Queue_Action_Kind_Delete: {
    steady_queue_slow_delete(arena, queue, action->id);
  } break;
  }
}

static void handle_queue_action_copy(Steady_Arena *arena, Steady_Queue_Copy *queue, Steady_Queue_Action *action) {
  switch (action->kind) {
  case Steady_Queue_Action_Kind_Push: {
    Steady_Queue_Copy_Node *node = arena_push(arena, sizeof(Steady_Queue_Copy_Node));
    node->value = action->value;
    steady_queue_copy_push(arena, queue, node);
  } break;
  case Steady_Queue_Action_Kind_PushFront: {
    Steady_Queue_Copy_Node *node = arena_push(arena, sizeof(Steady_Queue_Copy_Node));
    node->value = action->value;
    steady_queue_copy_push_front(arena, queue, node);
  } break;
  case Steady_Queue_Action_Kind_Pop: {
    steady_queue_copy_pop(arena, queue);
  } break;
  case Steady_Queue_Action_Kind_Delete: {
    steady_queue_copy_delete(arena, queue, action->id);
  } break;
  }
}








int main(void) {
  Arena *slow_arena = arena_alloc_reserve(10*1024*1024, 0);
  Arena *copy_arena = arena_alloc_reserve(10*1024*1024, 0);
  Arena *action_arena = arena_alloc_reserve(1*1024*1024, 0);

  SetPcgSeed(&Steady_Rng, 1, 1);

  // run tests
  U32 run_count = 4;
  for (U32 r = 0; r < run_count; ++r) {
    Steady_Queue_Slow slow_queue = (Steady_Queue_Slow){0};
    Steady_Queue_Copy copy_queue = (Steady_Queue_Copy){0};

    Steady_Queue_Action_Buffer action_buffer;
    action_buffer.count = pcg32_boundedrand_r(&Steady_Rng, 100);
    action_buffer.actions = steady_setup_queue_actions(action_arena, action_buffer.count);

    // perform slow actions
    for (U32 a = 0; a < action_buffer.count; ++a) {
      Steady_Queue_Action *action = action_buffer.actions + a;
      handle_queue_action_slow(slow_arena, &slow_queue, action);
    }
    // perform copy actions
    for (U32 a = 0; a < action_buffer.count; ++a) {
      Steady_Queue_Action *action = action_buffer.actions + a;
      handle_queue_action_copy(copy_arena, &copy_queue, action);
    }

    Assert(slow_queue.version_count == copy_queue.version_count);

    for (U32 v = 0; v < slow_queue.version_count; ++v) {
      Steady_Queue_Slow_Version *slow_version = steady_queue_slow_get_version(&slow_queue, v);
      Steady_Queue_Copy_Version *copy_version = steady_queue_copy_get_version(&copy_queue, v);

      Steady_Queue_Slow_Node *slow_node = slow_version->first;
      Steady_Queue_Copy_Node *copy_node = copy_version->first;

      for (;;) {
        B32 null_slow_node = slow_node == 0;
        B32 null_copy_node = copy_node == 0;

        if (null_slow_node || null_copy_node) {
          if (null_slow_node ^ null_copy_node) {
            printf("[ Error ] Mismatch node counts!\n");
          }
          break;
        }

        if (slow_node->value != copy_node->value) {
          printf("[ Error ] Mismatch node value!\n");
        }

        slow_node = slow_node->next;
        copy_node = copy_node->next;
      }
    }

    arena_pop_to(slow_arena, 0);
    arena_pop_to(copy_arena, 0);
  }

  return 0;
}
