#include "list.h"

#include <stdlib.h>

static ListItem songs[] = {
    { .name = "Song 1",  .item = (void*)1  },
    { .name = "Song 2",  .item = (void*)2  },
    { .name = "Song 3",  .item = (void*)3  },
    { .name = "Song 4",  .item = (void*)4  },
    { .name = "Song 5",  .item = (void*)5  },
    { .name = "Song 6",  .item = (void*)6  },
    { .name = "Song 7",  .item = (void*)7  },
    { .name = "Song 8",  .item = (void*)8  },
    { .name = "Song 9",  .item = (void*)9  },
    { .name = "Song 10", .item = (void*)10 },
    { .name = "Song 11", .item = (void*)11 },
    { .name = "Song 12", .item = (void*)12 },
    { .name = "Song 13", .item = (void*)13 },
    { .name = "Song 14", .item = (void*)14 },
    { .name = "Song 15", .item = (void*)15 },
    { .name = "Song 16", .item = (void*)16 },
};
static void* selected = (void*)1;
static int size = sizeof(songs) / sizeof(*songs);

static void* create_song() {
    return NULL;
}

void window_songs(float w, float h) {
    ListItem* items = songs;
    window_list(w, h, "Songs", &items, &size, &selected, create_song);
}
