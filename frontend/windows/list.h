#ifndef LIST_H
#define LIST_H

typedef struct {
    char* name;
    void* item;
} ListItem;

void window_list(float w, float h, const char* title, ListItem** arr, int* size, void** selected, void*(*create_item)());

#endif
