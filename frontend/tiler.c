#include "tiler.h"

#include <stdlib.h>

typedef struct TilerNode {
    enum {
        Leaf,
        SplitH,
        SplitV,
    } type;
    float x, y, w, h;
    struct TilerNode* left;
    struct TilerNode* right;
} TilerNode;

static TilerNode* master;

static void tiler_free(TilerNode* node) {
    if (!node) return;
    if (node->type != Leaf) {
        tiler_free(node->left);
        tiler_free(node->right);
    }
    free(node);
}

static TilerNode* tiler_get_node(int path, int* depth_ptr) {
    TilerNode* curr = master;
    int depth = 0;
    while (curr && curr->type != Leaf) {
        curr = (path >> depth) & 1 ? curr->right : curr->left;
        depth++;
    }
    if (depth_ptr) *depth_ptr = depth;
    return curr;
}

static TilerNode* tiler_make_node(float x, float y, float w, float h) {
    TilerNode* node = calloc(sizeof(TilerNode), 1);
    node->x = x;
    node->y = y;
    node->w = w;
    node->h = h;
    return node;
}

void tiler_deinit() {
    tiler_free(master);
}

void tiler_init(float width, float height) {
    tiler_deinit();
    master = tiler_make_node(0, 0, width, height);
}

bool tiler_splith(int id, float ratio, float offset, int* top, int* bottom) {
    int depth;
    TilerNode* node = tiler_get_node(id, &depth);
    if (!node) return false;
    float height = node->h * ratio + offset;
    node->type  = SplitH;
    node->left  = tiler_make_node(node->x, node->y, node->w, height);
    node->right = tiler_make_node(node->x, node->y + height, node->w, node->h - height);
    if (top)    *top    = id | (0 << depth);
    if (bottom) *bottom = id | (1 << depth);
    return true;
}

bool tiler_splitv(int id, float ratio, float offset, int* left, int* right) {
    int depth;
    TilerNode* node = tiler_get_node(id, &depth);
    if (!node) return false;
    float width = node->w * ratio + offset;
    node->type  = SplitV;
    node->left  = tiler_make_node(node->x, node->y, width, node->h);
    node->right = tiler_make_node(node->x + width, node->y, node->w - width, node->h);
    if (left)  *left  = id | (0 << depth);
    if (right) *right = id | (1 << depth);
    return true;
}

bool tiler_bounds(int id, float* x, float* y, float* w, float* h) {
    TilerNode* node = tiler_get_node(id, NULL);
    if (!node) return false;
    if (x) *x = node->x;
    if (y) *y = node->y;
    if (w) *w = node->w;
    if (h) *h = node->h;
    return true;
}
