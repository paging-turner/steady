
//////////////////////////////
// RB Tree
//////////////////////////////
RBNode *balance_node(RBNode *x, RBNode *y, RBNode *z, RBNode *b, RBNode *c, Color root_color);
NodeResult balance(RBNode *n, Color root_color);
NodeResult ins_recursive(RBNode *n, Key key, Value value);
RBNode *insert(RBNode *root, Key key, Value value);
RBNode *find(RBNode *root, Key key);
RBNode *rotate_right(RBNode *h);
RBNode *rotate_left(RBNode *h);
NodeResult equalize(RBNode *n, uint32_t dir);
NodeResult del_min(RBNode *n, Key *min_key_out, Value *min_val_out);
NodeResult del_recursive(RBNode *n, Key key);
RBNode *delete(RBNode *n, Key key);
void rb_delete(RBTree *tree, Key key);
void rb_insert(RBTree *tree, Key key, Value value);
bool rb_find(RBTree *tree, Key key, Value *out);


//////////////////////////////
// Persistent RB Tree
//////////////////////////////
// persistent-specific
void rb_node_release(RBTree *tree, RBNode *node);
RBTree *rb_alloc_tree(void);
RBNode *rb_alloc_node(RBTree *tree);
RBNode *node_clone(RBTree *tree, RBNode *src);
bool go_down(RBCursor *cursor, uint32_t dir);
bool go_up(RBCursor *cursor);
void rb_move_to_depth(RBCursor *cursor, int32_t depth);
RBCursor rb_init_cursor(RBTree *tree, RBNode *root, Arena *path_arena);
RBCursor rb_init_mut_cursor(RBTree *tree, Arena *path_arena);
RBNode *rb_commit(RBCursor *cursor);
RBNode *mutable_focus(RBCursor *cursor);
void update_child(RBCursor *cursor, uint32_t dir, RBNode *child);
void update_color(RBCursor *cursor, RBColor color);
void update_value(RBCursor *cursor, uint64_t value);
void update_key(RBCursor *cursor, uint64_t key);
void rb_replace(RBCursor *cursor, RBNode *node);
// similar api to rb-tree
void rb_balance_node(RBCursor *cursor, RBNode *x, RBNode *y, RBNode *z, RBNode *b, RBNode *c, RBColor root_color);
bool rb_balance(RBCursor *cursor, RBColor root_color);
bool rb_insert(RBCursor *cursor, uint64_t key, uint64_t value);
bool rb_search(RBNode *root, uint64_t key, uint64_t *val_out);
void rb_rotate(RBCursor *cursor, uint32_t dir);
bool rb_equalize(RBCursor *cursor, uint32_t dir);
bool rb_del_min(RBCursor *cursor, uint64_t *min_key_out, uint64_t *min_val_out);
bool rb_delete(RBCursor *cursor, uint64_t key);
