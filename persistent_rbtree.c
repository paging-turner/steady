#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <malloc.h>
#include <windows.h>

//Partially Persistent Red-Black Tree
//Imperative implementation of "Faster, Simpler Red-Black Trees" by Cameron Moy
//Implemented using Zippers
//https://ccs.neu.edu/~camoy/pub/red-black-tree.pdf
//https://ranger.uta.edu/~weems/NOTES5311/sigcse05.pdf
//https://github.com/zarif98sjs/RedBlackTree-An-Intuitive-Approach
//
//Just pedagogical. Arena implementation is a toy. 
//Arena would allocate from virtual memory in practice.
// cl -Zi -Od /INCREMENTAL:NO zipper.c

#define assert(expr) if(!(expr)) { *(char*)0 = 0; }
#define ArrayCount(a) (sizeof(a)/sizeof(a[0]))

//Fuck const forever
#pragma warning( disable : 4090 )

//Shitty arena implementation
typedef struct Arena Arena;
struct Arena {
    uint8_t *base;
    uint64_t size;
    uint64_t capacity;
    uint32_t flags;
};

typedef enum ArenaFlags ArenaFlags;
enum ArenaFlags {
    ARENA_FLAGS_NONE = 0,
    ARENA_FLAGS_NO_ZERO = (1 << 0),
};

uint8_t *arena_push(Arena *arena, uint64_t size, uint32_t flags) {
    uint8_t *result = 0;
    uint64_t remaining = arena->capacity - arena->size;
    if(size <= remaining) {
        result = &arena->base[arena->size];
        if(!(flags & ARENA_FLAGS_NO_ZERO)) {
            memset(result, 0, size);
        }
    }
    arena->size += size;
    return result;
}
#define push_array(a, c, T) (T*)arena_push(a, c*sizeof(T), 0);
#define push_array_no_zero(a, c, T) (T*)arena_push(a, c*sizeof(T), ARENA_FLAGS_NO_ZERO);

Arena *arena_alloc(uint64_t capacity) {
    uint8_t *base = malloc(capacity);
    Arena *arena = (Arena*)base;
    arena->base = base;
    arena->size = sizeof(*arena);
    arena->capacity = capacity;
    arena->flags = 0;
    
    return arena;
}

static Arena *temp_arena = 0;

#define RB_MAX_DEPTH 128

enum {
    LEFT = 0,
    RIGHT = 1,
    RB_CHILD_COUNT = 2,
};

typedef enum RBColor RBColor;
enum RBColor {
    RED = 0,
    BLACK = 1,
};

typedef struct RBNode RBNode;
struct RBNode {
    uint64_t key;
    uint64_t value;
    uint32_t color : 1;
    uint32_t timestamp : 31;
    union {
        RBNode *children[RB_CHILD_COUNT];
        RBNode *next_free;
    };
};

typedef struct RBTree RBTree;
struct RBTree {
    RBNode **roots;
    uint32_t roots_count;
    uint32_t roots_capacity;
    Arena *roots_arena;
    Arena *nodes_arena;
    uint32_t nodes_count;
    RBNode *free_list;
    uint32_t free_count;
};

typedef struct RBPathNode RBPathNode;
struct RBPathNode {
    RBNode *parent;
    uint32_t dir;
    bool mutable;
};

typedef struct RBCursor RBCursor;
struct RBCursor {
    uint32_t timestamp;
    
    RBNode *focus;
    uint32_t dir;
    bool mutable;

    RBTree *tree;

    int32_t depth;
    RBPathNode *path;
};

static const RBNode rb_nil = { .timestamp = 0, .color = BLACK, .children[LEFT] = &rb_nil, .children[RIGHT] = &rb_nil };

static void rb_node_release(RBTree *tree, RBNode *node) {
    if(node != &rb_nil) {
        bool mutable = node->timestamp == tree->roots_count;
        if(mutable) {
            node->next_free = tree->free_list;
            tree->free_list = node;
            tree->free_count += 1;
        }
    }
}

static RBTree *rb_alloc_tree(void) {
    Arena *nodes_arena = arena_alloc(sizeof(RBNode)*1000000 + 0x1000);
    RBTree *tree = push_array(nodes_arena, 1, RBTree);
    tree->nodes_arena = nodes_arena;
    Arena *roots_arena = arena_alloc(0x10000);
    tree->roots_arena = roots_arena;
    tree->roots_capacity = 0x100;
    tree->roots_count = 1;
    tree->roots = push_array(roots_arena, tree->roots_capacity, RBNode*);
    tree->roots[0] = &rb_nil;
    
    return tree;
}

static RBNode *rb_alloc_node(RBTree *tree) {
    RBNode *node = tree->free_list;
    if(!node) {
        node = push_array_no_zero(tree->nodes_arena, 1, RBNode);
        tree->nodes_count += 1;
    } else {
        tree->free_list = node->next_free;
        tree->free_count -= 1;
    }
    memset(node, 0, sizeof(*node));
    return node;
}

static RBNode *node_clone(RBTree *tree, RBNode *src) {
    RBNode *node = rb_alloc_node(tree);
    node->key = src->key;
    node->value = src->value;
    node->color = src->color;
    node->timestamp = tree->roots_count;
    for(uint32_t i = 0; i < RB_CHILD_COUNT; i++) {
        node->children[i] = src->children[i];
    }
    return node;
}

static bool go_down(RBCursor *cursor, uint32_t dir) {
    bool result = false;
    RBNode *current = cursor->focus;
        
    if(current != &rb_nil && cursor->depth < RB_MAX_DEPTH) {
        assert(dir < RB_CHILD_COUNT);
        RBNode *child = current->children[dir];
        bool mutable = cursor->mutable && child->timestamp == cursor->timestamp;
        
        cursor->depth += 1;
        RBPathNode *path_node = &cursor->path[cursor->depth-1];
        
        path_node->parent = current;
        path_node->dir = dir;
        path_node->mutable = cursor->mutable;

        cursor->focus = child;
        cursor->mutable = mutable;
        cursor->dir = dir;
        
        result = true;
    }
    return result;
}

static bool go_up(RBCursor *cursor) {
    bool result = false;
    if(cursor->depth != 0) {
        RBPathNode *path_node = &cursor->path[cursor->depth-1];
        cursor->depth -= 1;
        
        RBNode *current = cursor->focus;
        RBNode *parent = path_node->parent;
        RBNode *new_focus = parent;
        uint32_t dir = path_node->dir;
        bool mutable = path_node->mutable;
        
        RBNode *original_child = parent->children[dir];
        if(current != original_child) {
            if(!mutable) {
                RBNode *new_parent = node_clone(cursor->tree, parent);
                new_focus = new_parent;
                mutable = true;
            }
            new_focus->children[dir] = current;
            result = true;
        }
 
        cursor->focus = new_focus;
        cursor->mutable = mutable;
        cursor->dir = cursor->path[cursor->depth-1].dir;
    }
    return result;
}

static void rb_move_to_depth(RBCursor *cursor, int32_t depth) {
    if(depth < cursor->depth) {
        cursor->depth = depth;
        cursor->focus = cursor->path[depth].parent;
        cursor->mutable = cursor->path[depth].mutable;
        cursor->dir = cursor->path[depth-1].dir;
    }
}

static RBCursor rb_init_cursor(RBTree *tree, RBNode *root, Arena *path_arena) {
    RBCursor cursor = { .tree = tree };
    RBPathNode *path = push_array(path_arena, RB_MAX_DEPTH+1, RBPathNode);
    cursor.path = &path[1];
    cursor.focus = root;
    cursor.timestamp = tree->roots_count;
    cursor.mutable = false;

    return cursor;
}

static RBCursor rb_init_mut_cursor(RBTree *tree, Arena *path_arena) {
    RBNode *root = &rb_nil;
    if(tree->roots_count != 0) {
        root = tree->roots[tree->roots_count-1];
    }
    RBCursor cursor = rb_init_cursor(tree, root, path_arena);

    return cursor;
}

static RBNode *rb_commit(RBCursor *cursor) {
    while(cursor->depth != 0) {

    }
    RBNode *root = cursor->focus;
    RBTree *tree = cursor->tree;
    if(tree->roots_count >= tree->roots_capacity) {
        uint32_t added_cap = 0x100;
        push_array(tree->roots_arena, added_cap, RBNode*);
        tree->roots_capacity += added_cap;
    }
    
    tree->roots[tree->roots_count] = root;
    tree->roots_count += 1;
    cursor->timestamp = tree->roots_count;
    cursor->mutable = false;
    return root;
}


static RBNode *mutable_focus(RBCursor *cursor) {
    RBNode *node = cursor->focus;
    assert(node != &rb_nil);
    if(!cursor->mutable) {
        node = node_clone(cursor->tree, node);
        cursor->focus = node;
        cursor->mutable = true;
    }
    return node;
}

static void update_child(RBCursor *cursor, uint32_t dir, RBNode *child) {
    RBNode *node = mutable_focus(cursor);
    assert(dir < RB_CHILD_COUNT);
    node->children[dir] = child;
}

static void update_color(RBCursor *cursor, RBColor color) {
    RBNode *node = mutable_focus(cursor);
    node->color = color;
}

static void update_value(RBCursor *cursor, uint64_t value) {
    RBNode *node = mutable_focus(cursor);
    node->value = value;
}

static void update_key(RBCursor *cursor, uint64_t key) {
    RBNode *node = mutable_focus(cursor);
    node->key = key;
}

static void rb_replace(RBCursor *cursor, RBNode *node) {
    cursor->focus = node;
    cursor->mutable = node->timestamp == cursor->timestamp;
}

static void rb_rotate(RBCursor *cursor, uint32_t dir) {
    RBNode *h = mutable_focus(cursor);
    RBColor color = h->color;
    uint32_t opposite = !dir;
    RBNode *x = h->children[opposite];
    update_child(cursor, opposite, x->children[dir]);
    update_color(cursor, RED);
    rb_replace(cursor, x);
    update_child(cursor, dir, h);
    update_color(cursor, color);
}

static void 
rb_balance_node(RBCursor *cursor, RBNode *x, RBNode *y, RBNode *z, RBNode *b, RBNode *c, RBColor root_color) {
    rb_replace(cursor, y);
    update_color(cursor, root_color);
    update_child(cursor, LEFT, x);
    update_child(cursor, RIGHT, z);
    go_down(cursor, LEFT);
    update_color(cursor, BLACK);
    update_child(cursor, RIGHT, b);
    go_up(cursor);
    go_down(cursor, RIGHT);
    update_color(cursor, BLACK);
    update_child(cursor, LEFT, c);
    go_up(cursor);
}

uint32_t insert_depths[128] = {0};

static bool rb_balance(RBCursor *cursor, RBColor root_color) {
    RBNode *n = cursor->focus;

    RBNode *l = n->children[LEFT];
    RBNode *r = n->children[RIGHT];

    bool result = true;
    if(l->color == RED && l->children[LEFT]->color == RED) {
        RBNode *z = n;
        RBNode *y = z->children[LEFT];
        RBNode *x = y->children[LEFT];
        RBNode *b = x->children[RIGHT];
        RBNode *c = y->children[RIGHT];
        
        rb_balance_node(cursor, x, y, z, b, c, root_color);
    } else if(l->color == RED && l->children[RIGHT]->color == RED) {
        RBNode *z = n;
        RBNode *x = z->children[LEFT];
        RBNode *y = x->children[RIGHT];
        RBNode *b = y->children[LEFT];
        RBNode *c = y->children[RIGHT];
        
        rb_balance_node(cursor, x, y, z, b, c, root_color);
    } else if(r->color == RED && r->children[LEFT]->color == RED) {
        RBNode *x = n;
        RBNode *z = x->children[RIGHT];
        RBNode *y = z->children[LEFT];
        RBNode *b = y->children[LEFT];
        RBNode *c = y->children[RIGHT];
        
        rb_balance_node(cursor, x, y, z, b, c, root_color);
    } else if(r->color == RED && r->children[RIGHT]->color == RED) {
        RBNode *x = n;
        RBNode *y = x->children[RIGHT];
        RBNode *z = y->children[RIGHT];
        RBNode *b = y->children[LEFT];
        RBNode *c = z->children[LEFT];
        
        rb_balance_node(cursor, x, y, z, b, c, root_color);
    } else {
        result = false;
    }
    return result;
}

static bool rb_insert(RBCursor *cursor, uint64_t key, uint64_t value) {
    bool found = false;
    while(cursor->focus != &rb_nil) {
        RBNode *node = cursor->focus;
        
        if(key < node->key) {
            go_down(cursor, LEFT);
        } else if(key > node->key) {
            go_down(cursor, RIGHT);
        } else {
            update_value(cursor, value);
            found = true;
            break;
        }
    }
    if(!found) {
        RBNode *new_node = rb_alloc_node(cursor->tree);
        new_node->key = key;
        new_node->value = value;
        new_node->color = RED;
        new_node->timestamp = cursor->timestamp;

        for(uint32_t i = 0; i < RB_CHILD_COUNT; i++) {
            new_node->children[i] = &rb_nil;
        }
        
        cursor->focus = new_node;
        cursor->mutable = true;
    }
    bool do_balance = true;
    while(cursor->depth != 0) {
        bool keep_going = go_up(cursor);
        if(do_balance) {
            bool match = rb_balance(cursor, RED);
            do_balance = match || cursor->focus->color == RED;
        } else if(!keep_going) {
            //insert_depths[cursor->depth] += 1;
            rb_move_to_depth(cursor, 0);
            break;
        }
    }
    update_color(cursor, BLACK);
    
    return found;
}

static bool rb_equalize(RBCursor *cursor, uint32_t dir) {
    bool deficit = false;
    uint32_t begin_d = cursor->depth;
    uint32_t opposite = !dir;
    while(true) {
        RBNode *n = cursor->focus;
        if(n->children[opposite]->color == RED) {
            rb_rotate(cursor, dir);
            update_color(cursor, BLACK);
            go_down(cursor, dir);
        } else {
            break;
        }
    }
    RBNode *n = cursor->focus;
    if(n->children[opposite] != &rb_nil) {
        go_down(cursor, opposite);
        update_color(cursor, RED);
        go_up(cursor);
    }
    RBColor root_color = n->color;
    bool match = rb_balance(cursor, root_color);
    if(!match) {
        if(n->color == RED) {
            update_color(cursor, BLACK);
        } else {
            deficit = true;
        }
    }
    for(uint32_t d = cursor->depth; d > begin_d; d--) {
        go_up(cursor);
    }
    return deficit;
}

static bool rb_del_min(RBCursor *cursor, uint64_t *min_key_out, uint64_t *min_val_out) {
    bool deficit = false;
    uint32_t begin_d = cursor->depth;
    while(true) {
        RBNode *n = cursor->focus;
        if(n->children[LEFT] != &rb_nil) {
            go_down(cursor, LEFT);
        } else {
            break;
        }
    }
    RBNode *n = cursor->focus;
    *min_key_out = n->key;
    *min_val_out = n->value;
    RBNode *replacement = n->children[RIGHT];
    rb_replace(cursor, replacement);
    if(n->color == BLACK) {
        if(replacement->color == RED) {
            update_color(cursor, BLACK);
        } else {
            deficit = true;
        }
    }
    rb_node_release(cursor->tree, n);
    for(uint32_t d = cursor->depth; d > begin_d; d--) {
        go_up(cursor);
        if(deficit) {
            deficit = rb_equalize(cursor, LEFT);
        }
    }
    return deficit;
}

uint32_t delete_depths[128] = {0};

static bool rb_delete(RBCursor *cursor, uint64_t key) {
    bool found = false;
    while(cursor->focus != &rb_nil) {
        RBNode *node = cursor->focus;
        
        if(key < node->key) {
            go_down(cursor, LEFT);
        } else if(key > node->key) {
            go_down(cursor, RIGHT);
        } else {
            found = true;
            break;
        }
    }
    bool deficit = false;
    if(found) {
        RBNode *n = cursor->focus;
        if(n->children[RIGHT] == &rb_nil) {
            RBNode *replacement = n->children[LEFT];
            rb_replace(cursor, replacement);
            if(n->color == BLACK) {
                if(replacement->color == RED) {
                    update_color(cursor, BLACK);
                } else {
                    deficit = true;
                }
            }                    
            rb_node_release(cursor->tree, n);
        } else {
            uint64_t successor_key = 0;
            uint64_t successor_value = 0;
            go_down(cursor, RIGHT);
            deficit = rb_del_min(cursor, &successor_key, &successor_value);
            go_up(cursor);
            update_key(cursor, successor_key);
            update_value(cursor, successor_value);
            if(deficit) {
                deficit = rb_equalize(cursor, RIGHT);
            }
        }
    }
   while(cursor->depth != 0) {
        uint32_t dir = cursor->dir;
        bool keep_going = go_up(cursor);
        if(deficit) {
            deficit = rb_equalize(cursor, dir);
        } else if(!keep_going) {
            //delete_depths[cursor->depth] += 1;
            rb_move_to_depth(cursor, 0);
            break;    
        }
    }
    if(found && cursor->focus != &rb_nil) {
        update_color(cursor, BLACK);
    }
    return found;
}

static bool rb_search(RBNode *root, uint64_t key, uint64_t *val_out) {
    bool found = false;
    RBNode *n = root;
    while(n != &rb_nil) {
        if(key < n->key) {
            n = n->children[LEFT];
        } else if(key > n->key) {
            n = n->children[RIGHT];
        } else {
            found = true;
            if(val_out) {
                *val_out = n->value;
            }
            break;
        }
    }
    return found;
}

typedef struct Vec3 Vec3;
struct Vec3 {
    double v[3];
};  

static Vec3 lerp_3xf64(Vec3 a, Vec3 b, double t) {
    Vec3 result = {
        a.v[0]*(1-t) + b.v[0]*t,
        a.v[1]*(1-t) + b.v[1]*t,
        a.v[2]*(1-t) + b.v[2]*t,
    };
    return result;
}

static void print_rb_tree(RBTree *tree, RBNode *root) {
    uint32_t roots_count = tree->roots_count;
    
    RBNode **stack = push_array(temp_arena, RB_MAX_DEPTH+1, RBNode*);
    uint32_t depth = 0;
    
    stack[depth] = root;
    depth += 1;
    
    while(depth != 0) {
        RBNode *n = stack[depth-1];
        depth -= 1;
        
        if(n == &rb_nil) {
            return;
        }
        
        double t = (double)n->timestamp / (double)roots_count;
        Vec3 bg_col = lerp_3xf64(
            (Vec3){104.0/255.0, 143.0/255.0, 229.0/255.0}, 
            (Vec3){149.0/255.0, 228.0/255.0, 229.0/255.0}, 
            t
        );
        
        char *extra = n->color == RED ? ", shape=doublecircle, color=red" : "";
        char buf[1024];
        sprintf(buf, ", style=filled, fillcolor=\"%0.7f %0.7f %0.7f\"", bg_col.v[0], bg_col.v[1], bg_col.v[2]);
        char *bg_col_str = buf;
        printf("\tn_%lld_%p [label=\"%lld\\nts: %d\"%s%s];\n", n->key, n, n->key, n->timestamp, extra, bg_col_str);
        for(uint32_t i = 0; i < RB_CHILD_COUNT; i++) {
            RBNode *child = n->children[i];        
            if(child != &rb_nil) {
                char *extra = child->color == RED ? ", splines=ortho, minlen=0" : "";
                char *color = child->color == RED ? "red" : "black";
                uint32_t penwidth = child->color == RED ? 5 : 1;
                printf("\t" "n_%lld_%p -> n_%lld_%p [color=\"%s\", penwidth=%d, dir=none%s];\n", 
                    n->key, n, child->key, child, color, penwidth, extra);
            }
        }
        
        if(n->children[RIGHT] != &rb_nil) {
           stack[depth] = n->children[RIGHT];
           depth += 1;
        }
        if(n->children[LEFT] != &rb_nil) {
           stack[depth] = n->children[LEFT];
           depth += 1; 
        }
    }
}

uint64_t murmur64(uint64_t h) {
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdL;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53L;
    h ^= h >> 33;
    return h;
}

int main() {
    temp_arena = arena_alloc(0x4000); //would use scratch arena in practice

    RBTree *tree = rb_alloc_tree();
    
    RBCursor cursor = rb_init_mut_cursor(tree, temp_arena);
    
    bool gen_graphvis = true;
    
#define TEST 4
#if TEST==0 || TEST==1

    uint64_t *v = malloc(sizeof(uint64_t)*1000000);
#if TEST==0 
    for(size_t i = 0; i < 1000000; i++) {
        uint64_t h = murmur64(i);
        v[i] = h;
    }
#else
    for(size_t i = 0; i < 1000000; i++) {
        v[i] = i;
    }
#endif

    LARGE_INTEGER perf_freq = {0};
    QueryPerformanceFrequency(&perf_freq);

    LARGE_INTEGER begin_count = {0};
    QueryPerformanceCounter(&begin_count);

    for(size_t i = 0; i < 1000000; i++) {
        uint64_t h = v[i];
        rb_insert(&cursor, h, h);
    }

    for(size_t i = 20; i < 1000000-1; i++) {
        uint64_t h = v[i];

        rb_delete(&cursor, h);
    }
    RBNode *root1 = rb_commit(&cursor);
    
    for(uint32_t i = 0; i < 1000000; i++) {
        bool should_exist = !(i >= 20 && i < (1000000-1));
        uint64_t h = v[i];

        bool found = rb_search(root1, h, 0);
        assert(found == should_exist);
    }

    LARGE_INTEGER end_count = {0};
    QueryPerformanceCounter(&end_count);
    
    int64_t counter_elapsed = end_count.QuadPart - begin_count.QuadPart;
    int64_t ms_elapsed = 1000*counter_elapsed / perf_freq.QuadPart;
    
    printf("ms_elapsed = %lld\n", ms_elapsed);
    
    gen_graphvis = false;
#elif TEST==2
    uint32_t mid = 100;
    uint32_t end = 200;
    
    for(size_t i = 0; i < 100; i++) {
        rb_insert(&cursor, i, i);
    }
    RBNode *root1 = rb_commit(&cursor);

    for(uint32_t i = 20; i < 80; i++) {
        bool del_found = rb_delete(&cursor, i);
        assert(del_found);
    }
    RBNode *root2 = rb_commit(&cursor);
    
    for(uint32_t i = 0; i < 100; i++) {
        bool should_exist = true;
        bool found = rb_search(root1, i, 0);
        
        if(found != should_exist) {
            printf("NOT FOUND = %d\n", i);
        }
        assert(found == should_exist);
    }
    
    for(uint32_t i = 0; i < 100; i++) {
        bool should_exist = !(i >= 20 && i < 80);
        bool found = rb_search(root2, i, 0);
        assert(found == should_exist);
    }
#elif TEST==3
    
    uint64_t items[] = {55,33,4,878,2323,1,585,12,0};

    for(size_t i = 0; i < ArrayCount(items); i++) {
        rb_insert(&cursor, items[i], 0);
    }
    
    RBNode *root1 = rb_commit(&cursor);

    rb_insert(&cursor, 8, 8);
    
    RBNode *root2 = rb_commit(&cursor);
    
    for(size_t i = 0; i < ArrayCount(items); i++) {
        bool r1_found = rb_search(root1, items[i], 0);
        bool r2_found = rb_search(root2, items[i], 0);
        
        assert(r1_found);
        assert(r2_found);
    }
    assert(rb_search(root2, 8, 0));
    
#elif TEST==4
    for(size_t i = 0; i < 30; i++) {
        rb_insert(&cursor, i, i);
    }
    RBNode *root1 = rb_commit(&cursor);
    for(size_t i = 30; i < 60; i++) {
        rb_insert(&cursor, i, i);
    }
    RBNode *root2 = rb_commit(&cursor);
    
    for(size_t i = 20; i < 50; i++) {
        rb_delete(&cursor, i);
    }
    RBNode *root3 = rb_commit(&cursor); 
#elif TEST==5
    for(size_t i = 0; i < 20; i++) {
        rb_insert(&cursor, i, i);
    }
    RBNode *root1 = rb_commit(&cursor);
    for(size_t i = 20; i < 40; i++) {
        rb_insert(&cursor, i, i);
    }
    RBNode *root2 = rb_commit(&cursor);
    
    for(size_t i = 35; i < 40; i++) {
        rb_delete(&cursor, i);
    }
    RBNode *root3 = rb_commit(&cursor); 
#elif TEST==6
    for(size_t i = 0; i < 5; i++) {
        rb_insert(&cursor, i, i);
    }
    RBNode *root1 = rb_commit(&cursor);
    for(size_t i = 5; i < 10; i++) {
        rb_insert(&cursor, i, i);
    }
    RBNode *root2 = rb_commit(&cursor);
    for(size_t i = 5; i < 7; i++) {
        rb_delete(&cursor, i);
    }
    RBNode *root3 = rb_commit(&cursor); 
    
    for(size_t i = 0; i < 5; i++) {
        bool r1_found = rb_search(root1, i, 0);
        assert(r1_found);
    }
    
    for(size_t i = 0; i < 10; i++) {
        bool r2_found = rb_search(root2, i, 0);
        assert(r2_found);
    }
    
    for(size_t i = 0; i < 10; i++) {
        bool r3_found = rb_search(root3, i, 0);
        bool should_exist = !(i >= 5 && i < 7);

        assert(r3_found == should_exist);
    }
#elif TEST==7
    for(size_t i = 0; i < 2; i++) {
        rb_insert(&cursor, i, i);
    }
    RBNode *root1 = rb_commit(&cursor); 

    for(size_t i = 2; i < 4; i++) {
        rb_insert(&cursor, i, i);
    }
    RBNode *root2 = rb_commit(&cursor); 

    for(size_t i = 4; i < 6; i++) {
        rb_insert(&cursor, i, i);
    }
    RBNode *root3 = rb_commit(&cursor); 

    rb_delete(&cursor, 4);
    RBNode *root4 = rb_commit(&cursor); 

#endif

#undef TEST

    bool cluster = 0;

    if(gen_graphvis) {
        printf("strict digraph BST {\n");
        uint32_t roots_count = tree->roots_count;
        for(uint32_t i = 0; i < roots_count; i++) {
            RBNode *root = tree->roots[i];
            if(cluster) printf("subgraph cluster_%d {\n", i);
            print_rb_tree(tree, root);
            if(cluster) printf("}\n");
        }
        printf("}\n");
    }
    return 0;
}
#pragma warning( default : 4090 )
