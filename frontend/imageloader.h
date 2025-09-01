#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#include <SDL3/SDL.h>

typedef struct {
    unsigned char r, g, b, a;
} Pixel;

typedef struct {
    int width, height;
    Pixel* data;
    Pixel** pixels;
} Image;

#define get_pixel(x, y) pixels[y][x]

Image* img_get(const char* img);
Image* img_from_texture(SDL_Texture* texture);
SDL_Texture* img_get_texture(SDL_Renderer* renderer, const char* img);
SDL_Texture* img_generate_texture(SDL_Renderer* renderer, Image* img);

#endif