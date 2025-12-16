#define Steady_Set_Items_Per_Leaf 4
#define Steady_Set_Children_Per_Node 4


typedef struct Steady_Set_Item {
  U32 value;
} Steady_Set_Item;


typedef enum {
  Steady_Set_Node_Kind_Root,
  Steady_Set_Node_Kind_Inner,
  Steady_Set_Node_Kind_Leaf,
} Steady_Set_Node_Kind;


typedef struct Steady_Set_Node {
  Steady_Set_Node_Kind kind;
  union {
    struct {
      struct Steady_Set_Node *children[Steady_Set_Children_Per_Node];
      U64 max_node_id[Steady_Set_Children_Per_Node];
    };
    struct Steady_Set_Item items[Steady_Set_Items_Per_Leaf];
  };
} Steady_Set_Node;


typedef struct Steady_Set_Stack_Node {
  struct Steady_Set_Stack_Node *next;
  Steady_Set_Node *node;
  U32 index;
} Steady_Set_Stack_Node;


typedef struct Steady_Set_Stack {
  Steady_Set_Stack_Node *first;
  Steady_Set_Stack_Node *last;
} Steady_Set_Stack;


typedef struct Steady_Set {
  Steady_Set_Node *current_node;
  Steady_Set_Node *first_version;
  Steady_Set_Node *last_version;
} Steady_Set;



static U32 steady_set_get_child_count(Steady_Set_Node *node) {
  U32 count = 0;

  for (U32 i = 0; i < Steady_Set_Children_Per_Node; ++i) {
    Steady_Set_Node *child = node->children[i];

    if (child) {
      count += 1;
    }
    else {
      break;
    }
  }

  return count;
}

// TODO: Implement iterator for depth-first traversal of steady-set


/*
  B-Tree invariants of order m (the order is number of children per node)
    1. Every node has at most m children. (This is always true because of how we define the struct)
    2. Every node, except for the root and the leaves, has at least ceil(m/2) children.
    3. The root node has at least two children unless it is a leaf.
    4. All leaves appear on the same level.
    5. A non-leaf node with k children contains k−1 keys.
*/
static void steady_set_validate(Arena *arena, Steady_Set_Node *root) {
  Steady_Set_Stack stack = (Steady_Set_Stack){0};

  // TODO: We don't recycle stack-nodes, and we really should...
  Steady_Set_Stack_Node *stack_node = arena_push(arena, sizeof(Steady_Set_Stack_Node));
  stack_node->node = root;
  SLLStackPush(stack.first, stack_node);

  U32 depth = 0;

  if (root->kind != Steady_Set_Node_Kind_Root &&
      root->kind != Steady_Set_Node_Kind_Leaf) {
    printf("[ Validation Error ] Root is not a root kind, but a kind of %d\n", root->kind);
  }
  else {
    for (;;) {
      /* Steady_Set_Stack_Node *current_node = stack.first; */

      if (stack.first) {
        B32 should_descend = 0;
        B32 should_ascend = 0;


        switch (stack.first->node->kind) {
        case Steady_Set_Node_Kind_Root: {
          for (U32 i = 0; i < depth; ++i) {
            printf(" ");
          }
          printf("-\n");
          should_descend = 1;
        } break;
        case Steady_Set_Node_Kind_Inner: {
          for (U32 i = 0; i < depth; ++i) {
            printf(" ");
          }
          printf("-\n");
          should_descend = 1;
        } break;
        case Steady_Set_Node_Kind_Leaf: {
          for (U32 i = 0; i < Steady_Set_Items_Per_Leaf; ++i) {
            for (U32 i = 0; i < depth; ++i) {
              printf(" ");
            }
            printf("%d\n", stack.first->node->items[i].value);
          }
          should_ascend = 1;
        } break;
        }

        if (should_descend) {
          if (stack.first->index < Steady_Set_Children_Per_Node) {
            // TODO: We don't recycle stack-nodes, and we really should...
            Steady_Set_Node *child = stack.first->node->children[stack.first->index];
            if (child) {
              Steady_Set_Stack_Node *stack_node = arena_push(arena, sizeof(Steady_Set_Stack_Node));
              stack_node->node = child;
              SLLStackPush(stack.first, stack_node);
              depth += 1;
            }
            else {
              should_ascend = 1;
            }
          }
          else {
            should_ascend = 1;
          }
        }

        if (should_ascend) {
          SLLStackPop(stack.first);
          if (depth > 0) { depth -= 1; }
          if (stack.first) { stack.first->index += 1; }
        }
      }
      else {
        // Stack is empty, so we are done.
        break;
      }
    }
  }
}
