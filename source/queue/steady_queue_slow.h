/*
  This is a naive implementation for persistent lists/queues that is meant to be tested against.
  It trades efficiency for obviousness, in an attempt to prevent any potential bugs.
  The idea is that with this implementation, we can create more efficient implentations that match the behaviors of this code.
*/

#ifndef Steady_Queue_Slow_Value_Type
# define Steady_Queue_Slow_Value_Type void*
#endif



typedef struct Steady_Queue_Slow_Node Steady_Queue_Slow_Node;
typedef U64 Steady_Queue_Slow_Id;

struct Steady_Queue_Slow_Node {
  Steady_Queue_Slow_Id id;
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
  Steady_Queue_Slow_Id current_id;
  U32 version_count;
  Steady_Queue_Slow_Version *first_version;
  Steady_Queue_Slow_Version *last_version;
} Steady_Queue_Slow;



static void steady_queue_slow_copy_current_version(Steady_Arena *arena, Steady_Queue_Slow *queue) {
  // create new version
  Steady_Queue_Slow_Version *previous_version = queue->last_version;
  Steady_Queue_Slow_Version *next_version = steady_arena_push_size(arena, sizeof(Steady_Queue_Slow_Version));
  SLLQueuePush_NZ(queue->first_version, queue->last_version, next_version, next_version, 0);
  queue->version_count += 1;

  // copy nodes from previous version
  if (previous_version) {
    for (Steady_Queue_Slow_Node *n = previous_version->first; n != 0; n = n->next) {
      Steady_Queue_Slow_Node *copy_node = steady_arena_push_size(arena, sizeof(Steady_Queue_Slow_Node));
      *copy_node = *n;
      SLLQueuePush(next_version->first, next_version->last, copy_node);
    }
  }
}


static void steady_queue_slow_copy_current_version_and_skip(Steady_Arena *arena, Steady_Queue_Slow *queue, Steady_Queue_Slow_Id skip_id) {
  // create new version
  Steady_Queue_Slow_Version *previous_version = queue->last_version;
  Steady_Queue_Slow_Version *next_version = steady_arena_push_size(arena, sizeof(Steady_Queue_Slow_Version));
  SLLQueuePush_NZ(queue->first_version, queue->last_version, next_version, next_version, 0);
  queue->version_count += 1;

  // copy nodes from previous version
  if (previous_version) {
    for (Steady_Queue_Slow_Node *n = previous_version->first; n != 0; n = n->next) {
      if (n->id != skip_id) {
        Steady_Queue_Slow_Node *copy_node = steady_arena_push_size(arena, sizeof(Steady_Queue_Slow_Node));
        *copy_node = *n;
        SLLQueuePush(next_version->first, next_version->last, copy_node);
      }
    }
  }
}


static void steady_queue_slow_copy_current_version_and_set(Steady_Arena *arena, Steady_Queue_Slow *queue, Steady_Queue_Slow_Id id, Steady_Queue_Slow_Value_Type value) {
  // create new version
  Steady_Queue_Slow_Version *previous_version = queue->last_version;
  Steady_Queue_Slow_Version *next_version = steady_arena_push_size(arena, sizeof(Steady_Queue_Slow_Version));
  SLLQueuePush_NZ(queue->first_version, queue->last_version, next_version, next_version, 0);
  queue->version_count += 1;

  // copy nodes from previous version
  if (previous_version) {
    for (Steady_Queue_Slow_Node *n = previous_version->first; n != 0; n = n->next) {
      Steady_Queue_Slow_Node *copy_node = steady_arena_push_size(arena, sizeof(Steady_Queue_Slow_Node));
      *copy_node = *n;
      SLLQueuePush(next_version->first, next_version->last, copy_node);

      if (copy_node->id == id) {
        copy_node->value = value;
      }
    }
  }
}


void steady_queue_slow_push(Steady_Arena *arena, Steady_Queue_Slow *queue, Steady_Queue_Slow_Node *node) {
  steady_queue_slow_copy_current_version(arena, queue);

  // set and increment id
  node->id = queue->current_id;
  queue->current_id += 1;

  SLLQueuePush(queue->last_version->first, queue->last_version->last, node);
}


void steady_queue_slow_push_front(Steady_Arena *arena, Steady_Queue_Slow *queue, Steady_Queue_Slow_Node *node) {
  steady_queue_slow_copy_current_version(arena, queue);

  // set and increment id
  node->id = queue->current_id;
  queue->current_id += 1;

  SLLQueuePushFront(queue->last_version->first, queue->last_version->last, node);
}


void steady_queue_slow_pop(Steady_Arena *arena, Steady_Queue_Slow *queue) {
  if (queue->last_version && queue->last_version->first) {
    Steady_Queue_Slow_Id skip_id = queue->last_version->first->id;
    steady_queue_slow_copy_current_version_and_skip(arena, queue, skip_id);
  }
}


B32 steady_queue_slow_id_exists(Steady_Queue_Slow *queue, Steady_Queue_Slow_Id id) {
  B32 id_exists = 0;

  if (queue->last_version) {
    // @Speed look for id to delete
    for (Steady_Queue_Slow_Node *n = queue->last_version->first; n != 0; n = n->next) {
      if (n->id == id) {
        id_exists = 1;
      }
    }
  }

  return id_exists;
}


void steady_queue_slow_delete(Steady_Arena *arena, Steady_Queue_Slow *queue, Steady_Queue_Slow_Id id) {
  B32 id_exists = steady_queue_slow_id_exists(queue, id);

  if (id_exists) {
    steady_queue_slow_copy_current_version_and_skip(arena, queue, id);
  }
}


void steady_queue_slow_set(Steady_Arena *arena, Steady_Queue_Slow *queue, Steady_Queue_Slow_Id id, Steady_Queue_Slow_Value_Type value) {
  B32 id_exists = steady_queue_slow_id_exists(queue, id);

  if (id_exists) {
    steady_queue_slow_copy_current_version_and_set(arena, queue, id, value);
  }
}


void steady_queue_slow_undo(Steady_Arena *arena, Steady_Queue_Slow *queue) {
  Assert(!"TODO");
}

void steady_queue_slow_redo(Steady_Arena *arena, Steady_Queue_Slow *queue) {
  Assert(!"TODO");
}

Steady_Queue_Slow_Version *steady_queue_slow_get_version(Steady_Queue_Slow *queue, U32 version_id) {
  Steady_Queue_Slow_Version *version = 0;

  U32 current_version_id = 0;
  for (Steady_Queue_Slow_Version *v = queue->first_version; v != 0; v = v->next_version) {
    if (current_version_id == version_id) {
      version = v;
    }
    current_version_id += 1;
  }

  return version;
}
