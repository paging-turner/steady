/*
  This is a naive implementation for persistent lists/queues that is meant to be tested against.
  It trades efficiency for obviousness, in an attempt to prevent any potential bugs.
  The idea is that with this implementation, we can create more efficient implentations that match the behaviors of this code.
*/

#ifndef Steady_Queue_Slow_Value_Type
# define Steady_Queue_Slow_Value_Type void*
#endif

#if 0
#ifndef Steady_Arena
# error "Steady requires that an arena type be defined."
#endif
#ifndef steady_arena_push_size
# error "Steady requires that an arena function be defined that takes an arena and a size."
#endif
#endif

typedef struct Steady_Queue_Slow_Node Steady_Queue_Slow_Node;

struct Steady_Queue_Slow_Node {
  Steady_Queue_Slow_Value_Type value;
  Steady_Queue_Slow_Node *next;
};

typedef struct Steady_Queue_Slow_Version Steady_Queue_Slow_Version;
struct Steady_Queue_Slow_Version {
  Steady_Queue_Slow_Node *first;
  Steady_Queue_Slow_Node *last;
  Steady_Queue_Slow_Version *next_version;
};

typedef struct {
  Steady_Queue_Slow_Version *first_version;
  Steady_Queue_Slow_Version *last_version;
} Steady_Queue_Slow;

void steady_queue_slow_push(Steady_Arena *arena, Steady_Queue_Slow *queue, Steady_Queue_Slow_Node *node) {
  Steady_Queue_Slow_Version *current_version = queue->last_version;
  Steady_Queue_Slow_Version *next_version = steady_arena_push_size(arena, sizeof(Steady_Queue_Slow_Version));
  SLLQueuePush_NZ(queue->first_version, queue->last_version, next_version, next_version, 0);

  if (current_version) {
    for (Steady_Queue_Slow_Node *n = current_version->first; n != 0; n = n->next) {
      Steady_Queue_Slow_Node *copy_node = steady_arena_push_size(arena, sizeof(Steady_Queue_Slow_Node));
      *copy_node = *n;
      SLLQueuePush(next_version->first, next_version->last, copy_node);
    }
  }

  SLLQueuePush(next_version->first, next_version->last, node);
}

#if 0
void steady_queue_slow_push(Steady_Queue_Slow *queue, Steady_Queue_Slow_Node *node) {
  if (queue->first == 0 || queue->last == 0) {
    queue->first = queue->last = node;
  } else {
    queue->last->next = node;
    queue->last = node;
  }
  node->next = 0;
}

void steady_queue_slow_push_front(Steady_Queue_Slow *queue, Steady_Queue_Slow_Node *node) {
  if (queue->first == 0 || queue->last == 0) {
    queue->first = queue->last = node;
    node->next = 0;
  } else {
    node->next = queue->first;
    queue->first = node;
  }
}
#endif
