#include <iostream>
#include <fstream>
#include <assert.h>

#include <SDL2/SDL.h>
#include <glm/vec3.hpp>
#include "rapidjson/document.h"

#include "raytrace.hpp"

const int WIDTH = 640, HEIGHT = 480;

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
    }

    // Create Window and Renderer
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_FOREIGN, &window, &renderer);

    if (window == NULL || renderer == NULL) {
        std::cout << "Could not create window and/or renderer: " << SDL_GetError() << std::endl;
        return EXIT_FAILURE;
    }

    SDL_SetWindowTitle(window, "Game");

    // Texture to draw pixels to
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

    // Read scene file from json
    int file_length = 0;
    char *buff;
    std::ifstream f("scene.json", std::ifstream::binary | std::ios::ate); // Read file from end

    if (!f) {
        std::cout << "Failed to read scene.json" << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_SUCCESS;
    }

    file_length = f.tellg();
    buff = new char[file_length + 1];
    f.seekg(0, f.beg);
    f.read(buff, file_length);
    buff[file_length] = '\0';
    std::cout << "Reading scene.json with length " << file_length << std::endl;

    rapidjson::Document d;
    d.Parse(buff);
    assert(d.IsObject());

    // Pixel buffer
    Uint32 *pixels = new Uint32[WIDTH*HEIGHT];

    render(pixels, WIDTH, HEIGHT, d);

    if (SDL_UpdateTexture(texture, NULL, pixels, WIDTH * sizeof(Uint32)) < 0) {
        std::cout << "ERROR: " << SDL_GetError() << std::endl;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    if (SDL_RenderCopy(renderer, texture, NULL, NULL) < 0) {
        std::cout << "ERROR: " << SDL_GetError() << std::endl;
    }
    SDL_RenderPresent(renderer);

    delete[] pixels;
    delete[] buff;

    SDL_Event windowEvent;

    while (true) {
        if (SDL_PollEvent(&windowEvent)) {
            if (windowEvent.type == SDL_QUIT) {
                break;
            }
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}