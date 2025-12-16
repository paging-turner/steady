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
  U32 version_count;
  Steady_Queue_Copy_Version *first_version;
  Steady_Queue_Copy_Version *last_version;
} Steady_Queue_Copy;



static void steady_queue_copy_create_new_version(Steady_Arena *arena, Steady_Queue_Copy *queue) {
  Steady_Queue_Copy_Version *next_version = steady_arena_push_size(arena, sizeof(Steady_Queue_Copy_Version));
  SLLQueuePush_NZ(queue->first_version, queue->last_version, next_version, next_version, 0);
  queue->version_count += 1;
  /* printf("  copy version count += 1\n"); */
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
  if (previous_version) {
    node->next = previous_version->first;
    // @Speed set last pointer of current-version list
    Steady_Queue_Copy_Node *last_n = 0;
    for (Steady_Queue_Copy_Node *n = previous_version->first; n != 0; n = n->next) {
      last_n = n;
    }
    current_version->last = last_n;
  } else {
    node->next = 0;
  }
}


void steady_queue_copy_pop(Steady_Arena *arena, Steady_Queue_Copy *queue) {
  if (queue->last_version && queue->last_version->first) {
    // create new version
    Steady_Queue_Copy_Version *previous_version = queue->last_version;
    steady_queue_copy_create_new_version(arena, queue);
    Steady_Queue_Copy_Version *current_version = queue->last_version;

    // only pop if previous-version has at least two nodes, otherwise current-version is already zeroed
    if (previous_version->first != previous_version->last) {
      current_version->last = previous_version->last;
      current_version->first = previous_version->first->next;
    } else {
      B32 ummm = 0;
    }
  } else {
    B32 hmmm = 0;
  }
}


static B32 steady_queue_copy_id_exists(Steady_Queue_Copy *queue, Steady_Queue_Copy_Id id) {
  B32 id_exists = 0;

  if (queue->last_version) {
    // @Speed look for id to delete
    for (Steady_Queue_Copy_Node *n = queue->last_version->first; n != 0; n = n->next) {
      if (n->id == id) {
        id_exists = 1;
      }
    }
  }

  return id_exists;
}


void steady_queue_copy_delete(Steady_Arena *arena, Steady_Queue_Copy *queue, Steady_Queue_Copy_Id id) {
  // @Copypasta steady_queue_copy_set_value
  B32 id_exists = steady_queue_copy_id_exists(queue, id);

  if (id_exists) {
    // create new version
    Steady_Queue_Copy_Version *previous_version = queue->last_version;
    steady_queue_copy_create_new_version(arena, queue);
    Steady_Queue_Copy_Version *current_version = queue->last_version;

    // find the node to delete
    Steady_Queue_Copy_Node *previous_node = 0;
    for (Steady_Queue_Copy_Node *n = previous_version->first; n != 0; n = n->next) {
      if (n->id == id) {
        if (previous_node) {
          // copy nodes before the deleted node
          for (Steady_Queue_Copy_Node *c = previous_version->first; c != n; c = c->next) {
            Steady_Queue_Copy_Node *copy_node = steady_arena_push_size(arena, sizeof(Steady_Queue_Copy_Node));
            *copy_node = *c;
            SLLQueuePush(current_version->first, current_version->last, copy_node);
          }

          // point to the nodes after deleted node from previous version
          current_version->last->next = n->next;
          // @Speed set last pointer of current-version list
          Steady_Queue_Copy_Node *last_n = 0;
          for (Steady_Queue_Copy_Node *nn = n->next; nn != 0; nn = nn->next) {
            last_n = nn;
          }
          current_version->last = last_n;
        } else {
          // just like pop
          if (previous_version->first != previous_version->last) {
            current_version->last = previous_version->last;
            current_version->first = previous_version->first->next;
          }
        }
        break;
      }
      previous_node = n;
    }
  }
}


void steady_queue_copy_set(Steady_Arena *arena, Steady_Queue_Copy *queue, Steady_Queue_Copy_Id id, Steady_Queue_Copy_Value_Type value) {
  // @Copypasta steady_queue_copy_delete
  B32 id_exists = steady_queue_copy_id_exists(queue, id);

  if (id_exists) {
    // create new version
    Steady_Queue_Copy_Version *previous_version = queue->last_version;
    steady_queue_copy_create_new_version(arena, queue);
    Steady_Queue_Copy_Version *current_version = queue->last_version;

    // find the node to edit
    Steady_Queue_Copy_Node *previous_node = 0;
    for (Steady_Queue_Copy_Node *n = previous_version->first; n != 0; n = n->next) {
      if (n->id == id) {
        if (previous_node) {
          // copy nodes before the edited node
          for (Steady_Queue_Copy_Node *c = previous_version->first; c != n; c = c->next) {
            Steady_Queue_Copy_Node *copy_node = steady_arena_push_size(arena, sizeof(Steady_Queue_Copy_Node));
            *copy_node = *c;
            SLLQueuePush(current_version->first, current_version->last, copy_node);
          }
        }

        // copy current-node and edit the value
        Steady_Queue_Copy_Node *edit_node = steady_arena_push_size(arena, sizeof(Steady_Queue_Copy_Node));
        *edit_node = *n;
        edit_node->value = value;
        SLLQueuePush(current_version->first, current_version->last, edit_node);

        // point to the nodes after edited node from previous version
        current_version->last->next = n->next;
        // @Speed set last pointer of current-version list
        Steady_Queue_Copy_Node *last_n = 0;
        for (Steady_Queue_Copy_Node *nn = n->next; nn != 0; nn = nn->next) {
          last_n = nn;
        }
        current_version->last = last_n;

        break;
      }
      previous_node = n;
    }
  }
}


void steady_queue_copy_undo(Steady_Arena *arena, Steady_Queue_Copy *queue) {
  Assert(!"TODO");
}

void steady_queue_copy_redo(Steady_Arena *arena, Steady_Queue_Copy *queue) {
  Assert(!"TODO");
}


Steady_Queue_Copy_Version *steady_queue_copy_get_version(Steady_Queue_Copy *queue, U32 version_id) {
  Steady_Queue_Copy_Version *version = 0;

  U32 current_version_id = 0;
  for (Steady_Queue_Copy_Version *v = queue->first_version; v != 0; v = v->next_version) {
    if (current_version_id == version_id) {
      version = v;
    }
    current_version_id += 1;
  }

  return version;
}
