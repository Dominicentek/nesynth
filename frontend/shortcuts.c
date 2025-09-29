#include "shortcut.h"
#include "state.h"

#include <stdlib.h>

SHORTCUT(CTRL + N, // new project
    // todo
)

SHORTCUT(CTRL + O, // open project
    // todo
)

SHORTCUT(CTRL + S, // save project
    // todo
)

SHORTCUT(CTRL + SHIFT + S, // save project as
    // todo
)

SHORTCUT(CTRL + E, // export project
    // todo
)

SHORTCUT(CTRL + Z, // undo
    // todo
)

SHORTCUT(CTRL + SHIFT + Z, // redo
    // todo
)

SHORTCUT(CTRL + C, // copy
    // todo
)

SHORTCUT(CTRL + X, // cut
    // todo
)

SHORTCUT(CTRL + V, // paste
    // todo
)

SHORTCUT(DELETE, // delete selection
    // todo
)

SHORTCUT(SPACE, // play
    state.playing ^= 1;
)

SHORTCUT(SHIFT + SPACE, // play from start
    state.playing = true;
    *nesynth_beat_position(state.synth) = 0;
)

SHORTCUT(CTRL + SPACE, // play from pattern
    state.playing = true;
    *nesynth_beat_position(state.synth) = floorf(*nesynth_beat_position(state.synth) / 4) * 4;
)

SHORTCUT(CTRL + SHIFT + SPACE, // play from loop point
    *nesynth_beat_position(state.synth) = *nesynth_song_loop_point(state_song()) * 4;
)

SHORTCUT(CTRL + R, // rewind
    *nesynth_beat_position(state.synth) = 0;
)

SHORTCUT(CTRL + SHIFT + R, // rewind
    *nesynth_beat_position(state.synth) = floorf(*nesynth_beat_position(state.synth) / 4) * 4;
)

SHORTCUT(CTRL + Q, // quit
    // todo
    exit(0);
)
