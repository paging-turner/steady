#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
/* #include <sys/malloc.h> */
#include <stdlib.h>
#include <string.h>

//Imperative implementation of "Faster, Simpler Red-Black Trees" by Cameron Moy
//https://ccs.neu.edu/~camoy/pub/red-black-tree.pdf
//https://github.com/zarif98sjs/RedBlackTree-An-Intuitive-Approach
//https://ranger.uta.edu/~weems/NOTES5311/sigcse05.pdf

#define assert(expr) if(!(expr)) { *(char*)0 = 0; }

typedef uint64_t Key;
typedef uint64_t Value;

typedef enum {
    RED,
    BLACK,
} Color;

enum {
    RB_LEFT = 0,
    RB_RIGHT = 1,
};

typedef struct RBNode RBNode;
struct RBNode {
    Key key;
    Value value;
    Color color;
    union {
        struct {
            RBNode *left;
            RBNode *right;
        };
        RBNode *children[2];
    };
};

typedef struct RBTree RBTree;
struct RBTree {
    RBNode *root;
};

typedef struct NodeResult NodeResult;
struct NodeResult {
    bool status;
    RBNode *node;
};

bool is_red(RBNode *node) {
    bool result = node && node->color == RED;
    return result;
}

RBNode *balance_node(RBNode *x, RBNode *y, RBNode *z, RBNode *b, RBNode *c, Color root_color) {
    x->right = b;
    y->left = x;
    y->right = z;
    z->left = c;
    x->color = BLACK;
    y->color = root_color;
    z->color = BLACK;
    return y;
}

NodeResult balance(RBNode *n, Color root_color) {
    bool status = true;
    if(is_red(n->left) && is_red(n->left->left)) {
        n = balance_node(
            n->left->left,
            n->left,
            n,
            n->left->left->right,
            n->left->right,
            root_color
        );
    } else if(is_red(n->left) && is_red(n->left->right)) {
        n = balance_node(
            n->left,
            n->left->right,
            n,
            n->left->right->left,
            n->left->right->right,
            root_color
        );
    } else if(is_red(n->right) && is_red(n->right->left)) {
        n = balance_node(
            n,
            n->right->left,
            n->right,
            n->right->left->left,
            n->right->left->right,
            root_color
        );
    } else if(is_red(n->right) && is_red(n->right->right)) {
        n = balance_node(
            n,
            n->right,
            n->right->right,
            n->right->left,
            n->right->right->left,
            root_color
        );
    } else {
        status = false;
    }
    NodeResult result = { .status = status, .node = n };
    return result;
}

NodeResult ins_recursive(RBNode *n, Key key, Value value) {
    bool status = false;
    if(!n) {
        n = malloc(sizeof(RBNode));
        memset(n, 0, sizeof(*n));
        n->key = key;
        n->value = value;
        n->color = RED;
    } else if(key < n->key) {
        NodeResult res = ins_recursive(n->left, key, value);
        n->left = res.node;
        status = res.status;
    } else if(key > n->key) {
        NodeResult res = ins_recursive(n->right, key, value);
        n->right = res.node;
        status = res.status;
    } else {
        return (NodeResult){ .status = false, .node = n };
    }

    if(!status) {
        NodeResult res = balance(n, RED);
        n = res.node;
        if(res.status || is_red(n)) {
            status = true;
        }
    }
    NodeResult result = { .status = status, .node = n };
    
    return result;
}

RBNode *insert(RBNode *root, Key key, Value value) {
    NodeResult res = ins_recursive(root, key, value);
    RBNode *result = res.node;
    result->color = BLACK;
    return result;
}

RBNode *find(RBNode *root, Key key) {
    RBNode *result = 0;
    
    RBNode *next = root;
    while(next) {
        if(key < next->key) {
            next = next->left;
        } else if(key > next->key) {
            next = next->right;
        } else {
            result = next;
            break;
        }
    }
    return result;
}

static RBNode *rotate_right(RBNode *h) {
    RBNode *x = h->left;
    h->left = x->right;
    x->right = h;
    x->color = h->color;
    h->color = RED;
    return x;
}

static RBNode *rotate_left(RBNode *h) {
    RBNode *x = h->right;
    h->right = x->left;
    x->left = h;
    x->color = h->color;
    h->color = RED;
    return x;
}

static NodeResult equalize(RBNode *n, uint32_t dir) {
    bool status = false;
    uint32_t opposite = !dir;
    if(is_red(n->children[opposite])) {
        if(dir == RB_LEFT) {
            n = rotate_left(n);
        } else {
            n = rotate_right(n);
        }
        n->color = BLACK;
        NodeResult res = equalize(n->children[dir], dir);
        n->children[dir] = res.node;
        status = res.status;
    } else { 
        if(n->children[opposite]) {
            n->children[opposite]->color = RED;
        }
        Color root_color = n->color;
        NodeResult res = balance(n, root_color);
        n = res.node;
        if(!res.status) {
            if(is_red(n)) {
                n->color = BLACK;
            } else {
                status = true;
            }
        }
    }
    NodeResult result = { .status = status, .node = n };
    return result;
}

static NodeResult del_min(RBNode *n, Key *min_key_out, Value *min_val_out) {
    NodeResult result = { .status = false, .node = n };
    if(!n->left) {
        *min_key_out = n->key;
        *min_val_out = n->value;
        RBNode *replacement = n->right;
        bool deficit = false;
        if(!is_red(n)) {
            if(is_red(replacement)) {
                replacement->color = BLACK;
            } else {
                deficit = true;
            }
        }
        result = (NodeResult){ .status = deficit, .node = replacement };
        free(n);
    } else {
        NodeResult res = del_min(n->left, min_key_out, min_val_out);
        n->left = res.node;
        if(res.status) {
            result = equalize(n, RB_LEFT);
        }
    }
    return result;
}

static NodeResult del_recursive(RBNode *n, Key key) {
    NodeResult result = { .status = false, .node = n };
    if(!n) {
    } else if(key < n->key) {
        NodeResult res = del_recursive(n->left, key);
        n->left = res.node;
        if(res.status) {
            result = equalize(n, RB_LEFT);
        }
    } else if(key > n->key) {
        NodeResult res = del_recursive(n->right, key);
        n->right = res.node;
        if(res.status) {
            result = equalize(n, RB_RIGHT);
        }
    } else {
        if(!n->right) {
            RBNode *replacement = n->left;
            bool deficit = false;
            if(!is_red(n)) {
                if(is_red(replacement)) {
                    replacement->color = BLACK;
                } else {
                    deficit = true;
                }
            }
            result = (NodeResult){ .status = deficit, .node = replacement };
            free(n);
        } else {
            Key successor_key = 0;
            Value successor_value = 0;
            NodeResult res = del_min(n->right, &successor_key, &successor_value);
            n->key = successor_key;
            n->value = successor_value;
            n->right = res.node;
            if(res.status) {
                result = equalize(n, RB_RIGHT);
            }
        }
    }
    return result;
}

static RBNode *delete(RBNode *n, Key key) {
    NodeResult result = del_recursive(n, key);
    if(result.node) {
        result.node->color = BLACK;
    }
    return result.node;
}

void rb_delete(RBTree *tree, Key key) {
    tree->root = delete(tree->root, key);
}

void rb_insert(RBTree *tree, Key key, Value value) {
    tree->root = insert(tree->root, key, value);
}

bool rb_find(RBTree *tree, Key key, Value *out) {
    RBNode *node = find(tree->root, key);
    Value value = node ? node->value : 0;
    if(out) {
        *out = value;
    }
    return (node != 0);
}

static void print_rb_tree_recurse(RBNode *node) {
    if(!node) return;
    char *extra = is_red(node) ? ", shape=doublecircle, color=red" : "";
    printf("\tn_%lld_%p [label=\"%lld\"%s];\n", node->key, node, node->key, extra);
    
    for(uint32_t i = 0; i < 2; i++) {
        RBNode *child = node->children[i];        
        if(child) {
            char *extra = child->color == RED ? ", splines=ortho, minlen=0" : "";
            char *color = child->color == RED ? "red" : "black";
            uint32_t penwidth = child->color == RED ? 5 : 1;
            printf("\t" "n_%lld_%p -> n_%lld_%p [color=\"%s\", penwidth=%d, dir=none%s];\n", 
                node->key, node, child->key, child, color, penwidth, extra);
            //print_rb_tree_recurse(child);
        }
    }
    for(uint32_t i = 0; i < 2; i++) {
        RBNode *child = node->children[i];        
        if(child) {
            print_rb_tree_recurse(child);
        }
    }
}

static void print_rb_tree(RBNode *node) {
    printf("strict digraph BST {\n");
    print_rb_tree_recurse(node);
    printf("}\n");
}

#if 0
int main(int argc, char **argv) {
    RBTree tree = {0};
    
    for(size_t i = 0; i < 100; i++) {
        rb_insert(&tree, i, i);
    }
    
    for(size_t i = 20; i < 80; i++) {
        rb_delete(&tree, i);
    }
    
    print_rb_tree(tree.root);
    
    bool found = false;
    Value val = 0;
    found = rb_find(&tree, 50, &val);
    
    return 0;
}
#endif
