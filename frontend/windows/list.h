#ifndef LIST_H
#define LIST_H

typedef struct {
    char* name;
    void* item;
} ListItem;

typedef struct {
    int num_items;
    ListItem* items;
} List;

void window_list(float w, float h, const char* title, List* list, void* selected, const char* menu, void(*menu_handler)(int index, void* data), void(*create_item)());

#endif
