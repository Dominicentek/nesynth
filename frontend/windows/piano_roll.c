#include "ui.h"

void window_piano_roll(float w, float h) {
    ui_item(300, 100);
        ui_text(8, 8,  "the quick brown fox jumps over the lazy dog");
        ui_text(8, 16, "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG");
        ui_text(8, 24, "0123456789!\"#$%&'()*+,-./:;<=>?[\\]^_`{|}~");
    ui_end();
}
