#ifndef LIST_H
#define LIST_H

typedef struct {
    int num_items;
    int color_seed;
    struct ListItem* items;
} List;

typedef struct ListItem {
    char* name;
    void* item;
    int color;
    List nested_list;
} ListItem;

void window_list(float w, float h, const char* title, List* list, int* selected, const char* menu, void(*menu_handler)(int index, void* data), void(*create_item)());

#endif
