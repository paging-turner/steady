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



#if 0
# define Steady_Debug_Print(...) printf(__VA_ARGS__)
#else
# define Steady_Debug_Print(...)
#endif



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

#define Steady_Debug_Manual_Actions 0

static Steady_Queue_Action *steady_setup_queue_actions(Arena *arena, U32 action_count) {
#if Steady_Debug_Manual_Actions
  Steady_Queue_Action *actions = arena_push(arena, 3 * sizeof(Steady_Queue_Action));
  if (actions) {
    actions[0].kind = Steady_Queue_Action_Kind_Push;
    actions[0].value = 2;
    actions[1].kind = Steady_Queue_Action_Kind_Push;
    actions[1].value = 34;
    actions[2].kind = Steady_Queue_Action_Kind_Push;
    actions[2].value = 61;
    actions[3].kind = Steady_Queue_Action_Kind_Pop;
    actions[4].kind = Steady_Queue_Action_Kind_Delete;
    actions[4].id = 0;
    actions[5].kind = Steady_Queue_Action_Kind_Push;
    actions[5].value = 79;
    actions[6].kind = Steady_Queue_Action_Kind_Push;
    actions[6].value = 77;
    actions[7].kind = Steady_Queue_Action_Kind_Pop;
    actions[8].kind = Steady_Queue_Action_Kind_Delete;
    actions[8].id = 2;
    actions[9].kind = Steady_Queue_Action_Kind_PushFront;
    actions[9].value = 25;
    actions[10].kind = Steady_Queue_Action_Kind_Delete;
    actions[10].id = 1;
    actions[11].kind = Steady_Queue_Action_Kind_Delete;
    actions[11].id = 3;
    actions[12].kind = Steady_Queue_Action_Kind_Pop;
  }
#else
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
        action->value = pcg32_boundedrand_r(&Steady_Rng, 99);
        opl_max_node_id += 1;
      } else if (action_kind == Steady_Queue_Action_Kind_Delete) {
        action->id = pcg32_boundedrand_r(&Steady_Rng, opl_max_node_id);
      }
    }
  }
#endif

  return actions;
}

static void steady_debug_print_queue_actions(Steady_Queue_Action_Buffer *action_buffer) {
  // debug print out actions
  Steady_Debug_Print("Actions[%d]:\n", action_buffer->count);
  for (U32 i = 0; i < action_buffer->count; ++i) {
    Steady_Queue_Action *action = action_buffer->actions + i;

    switch (action->kind) {
    case Steady_Queue_Action_Kind_Push: {
      Steady_Debug_Print("  Push %d\n", action->value);
    } break;
    case Steady_Queue_Action_Kind_PushFront: {
      Steady_Debug_Print("  PushFront %d\n", action->value);
    } break;
    case Steady_Queue_Action_Kind_Pop: {
      Steady_Debug_Print("  Pop\n");
    } break;
    case Steady_Queue_Action_Kind_Delete: {
      Steady_Debug_Print("  Delete %llu\n", action->id);
    } break;
    }
  }
}



static void handle_queue_action_slow(Steady_Arena *arena, Steady_Queue_Slow *queue, Steady_Queue_Action *action) {
  switch (action->kind) {
  case Steady_Queue_Action_Kind_Push: {
    Steady_Debug_Print("slow push %d\n", action->value);
    Steady_Queue_Slow_Node *node = arena_push(arena, sizeof(Steady_Queue_Slow_Node));
    node->value = action->value;
    steady_queue_slow_push(arena, queue, node);
  } break;
  case Steady_Queue_Action_Kind_PushFront: {
    Steady_Debug_Print("slow push-front %d\n", action->value);
    Steady_Queue_Slow_Node *node = arena_push(arena, sizeof(Steady_Queue_Slow_Node));
    node->value = action->value;
    steady_queue_slow_push_front(arena, queue, node);
  } break;
  case Steady_Queue_Action_Kind_Pop: {
    Steady_Debug_Print("slow pop\n");
    steady_queue_slow_pop(arena, queue);
  } break;
  case Steady_Queue_Action_Kind_Delete: {
    Steady_Debug_Print("slow delete %llu\n", action->id);
    steady_queue_slow_delete(arena, queue, action->id);
  } break;
  }
}

static void handle_queue_action_copy(Steady_Arena *arena, Steady_Queue_Copy *queue, Steady_Queue_Action *action) {
  switch (action->kind) {
  case Steady_Queue_Action_Kind_Push: {
    Steady_Debug_Print("copy push %d\n", action->value);
    Steady_Queue_Copy_Node *node = arena_push(arena, sizeof(Steady_Queue_Copy_Node));
    node->value = action->value;
    steady_queue_copy_push(arena, queue, node);
  } break;
  case Steady_Queue_Action_Kind_PushFront: {
    Steady_Debug_Print("copy push-front %d\n", action->value);
    Steady_Queue_Copy_Node *node = arena_push(arena, sizeof(Steady_Queue_Copy_Node));
    node->value = action->value;
    steady_queue_copy_push_front(arena, queue, node);
  } break;
  case Steady_Queue_Action_Kind_Pop: {
    Steady_Debug_Print("copy pop\n");
    steady_queue_copy_pop(arena, queue);
  } break;
  case Steady_Queue_Action_Kind_Delete: {
    Steady_Debug_Print("copy delete %llu\n", action->id);
    steady_queue_copy_delete(arena, queue, action->id);
  } break;
  }
}




static void steady_queue_slow_print_version(Steady_Queue_Slow *queue, S32 version) {
  Steady_Queue_Slow_Version *v = 0;
  if (version == -1) {
    v = queue->last_version;
  } else {
    v = steady_queue_slow_get_version(queue, version);
  }
  Steady_Debug_Print("  [");
  if (v) {
    for (Steady_Queue_Slow_Node *n = v->first; n != 0; n = n->next) {
      Steady_Debug_Print("%d(%llu)%s", n->value, n->id, n->next?"->":"");
    }
  }
  Steady_Debug_Print("]\n");
}

static void steady_queue_copy_print_version(Steady_Queue_Copy *queue, S32 version) {
  Steady_Queue_Copy_Version *v = 0;
  if (version == -1) {
    v = queue->last_version;
  } else {
    v = steady_queue_copy_get_version(queue, version);
  }
  Steady_Debug_Print("  [");
  if (v) {
    for (Steady_Queue_Copy_Node *n = v->first; n != 0; n = n->next) {
      Steady_Debug_Print("%d(%llu)%s", n->value, n->id, n->next?"->":"");
    }
  }
  Steady_Debug_Print("]\n");
}





int main(void) {
  Arena *slow_arena = arena_alloc_reserve(10*1024*1024, 0);
  Arena *copy_arena = arena_alloc_reserve(10*1024*1024, 0);
  Arena *action_arena = arena_alloc_reserve(1*1024*1024, 0);

  SetPcgSeed(&Steady_Rng, 1, 1);

  // run tests
  U32 run_count = 10000;
  for (U32 r = 0; r < run_count; ++r) {
    if (r % 1000 == 0) {
      printf("\n%d", r);
    }
    Steady_Queue_Slow slow_queue = (Steady_Queue_Slow){0};
    Steady_Queue_Copy copy_queue = (Steady_Queue_Copy){0};

    Steady_Queue_Action_Buffer action_buffer;
#if Steady_Debug_Manual_Actions
    action_buffer.count = 13;
#else
    action_buffer.count = 8+pcg32_boundedrand_r(&Steady_Rng, 24);
#endif
    action_buffer.actions = steady_setup_queue_actions(action_arena, action_buffer.count);

    /* steady_debug_print_queue_actions(&action_buffer); */

    // perform slow actions
    for (U32 a = 0; a < action_buffer.count; ++a) {
      Steady_Queue_Action *action = action_buffer.actions + a;
      handle_queue_action_slow(slow_arena, &slow_queue, action);
      steady_queue_slow_print_version(&slow_queue, -1);
    }
    // perform copy actions
    for (U32 a = 0; a < action_buffer.count; ++a) {
      Steady_Queue_Action *action = action_buffer.actions + a;
      handle_queue_action_copy(copy_arena, &copy_queue, action);
      steady_queue_copy_print_version(&copy_queue, -1);
    }

    if (slow_queue.version_count != copy_queue.version_count) {
      printf("[ Error ] Version mismatch!\n");
    }
    Assert(slow_queue.version_count == copy_queue.version_count);

    B32 error = 0;
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
            error = 1;
          }
          break;
        }

        if (slow_node->value != copy_node->value) {
          printf("[ Error ] Mismatch node value!\n");
          error = 1;
        }

        slow_node = slow_node->next;
        copy_node = copy_node->next;
      }
    }

    if (error) {
      break;
    }

    arena_pop_to(slow_arena, 0);
    arena_pop_to(copy_arena, 0);
    arena_pop_to(action_arena, 0);
  }

  return 0;
}
