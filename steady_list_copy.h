/*
  This list/queue implementation copies nodes up to the root when necessary.
  Stack pushes should be efficient while queue pushes are *not* efficient.
*/

#ifndef Steady_Queue_Copy_Value_Type
# define Steady_Queue_Copy_Value_Type void*
#endif



typedef struct Steady_Queue_Copy_Node Steady_Queue_Copy_Node;
typedef U64 Steady_Queue_Copy_Id;

struct Steady_Queue_Copy_Node {
  Steady_Queue_Copy_Id id;
  Steady_Queue_Copy_Value_Type value;
  Steady_Queue_Copy_Node *next;
  Steady_Queue_Copy_Node *prev;
};

typedef struct Steady_Queue_Copy_Version Steady_Queue_Copy_Version;
struct Steady_Queue_Copy_Version {
  Steady_Queue_Copy_Node *first;
  Steady_Queue_Copy_Node *last;
  Steady_Queue_Copy_Version *next_version;
};

typedef struct {
  Steady_Queue_Copy_Id current_id;
  Steady_Queue_Copy_Version *first_version;
  Steady_Queue_Copy_Version *last_version;
} Steady_Queue_Copy;



static void steady_queue_copy_create_new_version(Steady_Arena *arena, Steady_Queue_Copy *queue) {
  Steady_Queue_Copy_Version *next_version = steady_arena_push_size(arena, sizeof(Steady_Queue_Copy_Version));
  SLLQueuePush_NZ(queue->first_version, queue->last_version, next_version, next_version, 0);
}

void steady_queue_copy_push(Steady_Arena *arena, Steady_Queue_Copy *queue, Steady_Queue_Copy_Node *node) {
  // create new version
  Steady_Queue_Copy_Version *previous_version = queue->last_version;
  steady_queue_copy_create_new_version(arena, queue);
  Steady_Queue_Copy_Version *current_version = queue->last_version;

  // copy nodes from previous version
  if (previous_version) {
    for (Steady_Queue_Copy_Node *n = previous_version->first; n != 0; n = n->next) {
      Steady_Queue_Copy_Node *copy_node = steady_arena_push_size(arena, sizeof(Steady_Queue_Copy_Node));
      *copy_node = *n;
      SLLQueuePush(current_version->first, current_version->last, copy_node);
    }
  }

  // set and increment id
  node->id = queue->current_id;
  queue->current_id += 1;

  // add node to current version
  SLLQueuePush(current_version->first, current_version->last, node);
}


void steady_queue_copy_push_front(Steady_Arena *arena, Steady_Queue_Copy *queue, Steady_Queue_Copy_Node *node) {
  // create new version
  Steady_Queue_Copy_Version *previous_version = queue->last_version;
  steady_queue_copy_create_new_version(arena, queue);
  Steady_Queue_Copy_Version *current_version = queue->last_version;

  // set and increment id
  node->id = queue->current_id;
  queue->current_id += 1;

  // add node to current version
  SLLQueuePush(current_version->first, current_version->last, node);
  node->next = previous_version ? previous_version->first : 0;
}


void steady_queue_copy_pop(Steady_Arena *arena, Steady_Queue_Copy *queue) {
}


void steady_queue_copy_delete(Steady_Arena *arena, Steady_Queue_Copy *queue, Steady_Queue_Copy_Id id) {
}
