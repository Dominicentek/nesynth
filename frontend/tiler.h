#ifndef TILER_H
#define TILER_H

#define ROOT_NODE 0

#include <stdbool.h>

void tiler_deinit();
void tiler_init(float width, float height);
bool tiler_splith(int id, float ratio, float pixel_offset, int* top, int* bottom);
bool tiler_splitv(int id, float ratio, float pixel_offset, int* left, int* right);
bool tiler_bounds(int id, float* x, float* y, float* w, float* h);

#endif
