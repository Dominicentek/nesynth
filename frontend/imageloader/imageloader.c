#include "imageloader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

typedef struct {
    int len;
    const char* name;
    const char* data;
    Image* image;
    SDL_Texture* texture;
} ImageData;
ImageData image_data[] = {
#include "images/all.h"
};
static int num_images = sizeof(image_data) / sizeof(*image_data);

static ImageData* get(const void* ptr) {
    if (ptr == NULL) return NULL;
    for (int i = 0; i < num_images; i++) {
        if (image_data[i].image == ptr || image_data[i].texture == ptr || strcmp(image_data[i].name, ptr) == 0) return &image_data[i];
    }
    return NULL;
}

Image* img_get(const char* path) {
    ImageData* cache = get(path);
    if (!cache) return NULL;
    if (cache->image) return cache->image;
    int channels;
    Image* img = malloc(sizeof(Image));
    img->data = (void*)stbi_load_from_memory((void*)cache->data, cache->len, &img->width, &img->height, &channels, 4);
    img->pixels = malloc(sizeof(Pixel*) * img->height);
    for (int i = 0; i < img->height; i++) img->pixels[i] = &img->data[i * img->width];
    cache->image = img;
    return img;
}

Image* img_from_texture(SDL_Texture* texture) {
    ImageData* cache = get(texture);
    if (!cache) return NULL;
    return cache->image;
}

SDL_Texture* img_get_texture(SDL_Renderer* renderer, const char* img) {
    return img_generate_texture(renderer, img_get(img));
}

SDL_Texture* img_generate_texture(SDL_Renderer* renderer, Image* img) {
    ImageData* cache = get(img);
    if (!cache) return NULL;
    if (cache->texture) return cache->texture;
    SDL_Surface* surface = SDL_CreateSurfaceFrom(img->width, img->height, SDL_PIXELFORMAT_ABGR8888, img->data, 4 * img->width);
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    SDL_DestroySurface(surface);
    cache->texture = tex;
    return tex;
}
