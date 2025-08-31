NAME = nesynth

FRONTEND_ENABLE = 1
FRONTEND_LINK_STATICALLY = 0

FRONTEND = frontend
BUILD = obj
LIBS = sdl3

CFLAGS = -I. -Ifrontend -fPIC -g
LFLAGS = -lm
