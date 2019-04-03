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

    /* Create scene */

    int num_objects = d["objects"]["spheres"].Size() + d["objects"]["triangles"].Size();
    int num_lights = d["lights"].Size();
    int fov = d["camera"]["fov"].GetFloat();
    Scene scene{WIDTH, HEIGHT, fov, num_objects, num_lights};

    // Create a camera facing forward
    scene.camera.halfWidth = glm::tan((scene.camera.fov / 2) * (M_PI / 180)); // A misnomer, but whatever
    scene.camera.halfHeight = scene.camera.halfWidth * (WIDTH/HEIGHT);
    scene.camera.pixelWidth = (scene.camera.halfWidth * 2) / (WIDTH - 1);
    scene.camera.pixelHeight = (scene.camera.halfHeight * 2) / (HEIGHT - 1);

    // Move camera
    vec3 cameraPos{d["camera"]["x"].GetFloat(), d["camera"]["y"].GetFloat(), d["camera"]["z"].GetFloat()};
    vec3 cameraPoint{d["camera"]["toX"].GetFloat(), d["camera"]["toY"].GetFloat(), d["camera"]["toZ"].GetFloat()};
    scene.camera.move(cameraPos, cameraPoint);

    // Create vector of scene objects

    // Get sphere objects from json document
    int i = 0;
    for (auto &s : d["objects"]["spheres"].GetArray()) {
        Sphere *sph = new Sphere{   vec3{s["x"].GetFloat(), s["y"].GetFloat(), s["z"].GetFloat()},
                                    s["radius"].GetFloat(),
                                    vec3{s["r"].GetFloat(), s["g"].GetFloat(), s["b"].GetFloat()},
                                    d["materials"][s["material"].GetInt()]["lambert"].GetFloat(),
                                    d["materials"][s["material"].GetInt()]["specular"].GetFloat()};
        scene.objects[i++] = sph;
    }

    // Get triangle objects from json document
    for (auto &t : d["objects"]["triangles"].GetArray()) {
        Triangle *tri = new Triangle{   vec3{t["v1"]["x"].GetFloat(), t["v1"]["y"].GetFloat(), t["v1"]["z"].GetFloat()},
                                        vec3{t["v2"]["x"].GetFloat(), t["v2"]["y"].GetFloat(), t["v2"]["z"].GetFloat()},
                                        vec3{t["v3"]["x"].GetFloat(), t["v3"]["y"].GetFloat(), t["v3"]["z"].GetFloat()},
                                        vec3{t["r"].GetFloat(), t["g"].GetFloat(), t["b"].GetFloat()},
                                        d["materials"][t["material"].GetInt()]["lambert"].GetFloat(),
                                        d["materials"][t["material"].GetInt()]["specular"].GetFloat()};
        scene.objects[i++] = tri;
    }

    // Get scene lights from json document
    i = 0;
    for (auto &l : d["lights"].GetArray()) {
        Light *lgt = new Light{ vec3{l["x"].GetFloat(), l["y"].GetFloat(), l["z"].GetFloat()},
                                vec3{l["r"].GetFloat(), l["g"].GetFloat(), l["b"].GetFloat()}};
        scene.lights[i++] = lgt;
    }

    // Anti-Aliasing
    scene.AA = d["AA"].GetInt();

    // Pixel buffer
    Uint32 *pixels = new Uint32[WIDTH*HEIGHT];

    // Render initial scene
    render(pixels, scene);

    if (SDL_UpdateTexture(texture, NULL, pixels, WIDTH * sizeof(Uint32)) < 0) {
        std::cout << "ERROR: " << SDL_GetError() << std::endl;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    if (SDL_RenderCopy(renderer, texture, NULL, NULL) < 0) {
        std::cout << "ERROR: " << SDL_GetError() << std::endl;
    }
    SDL_RenderPresent(renderer);

    SDL_Event event;

    glm::vec3 move{0.0, 0.0, 0.0};
    glm::vec3 angle{0.0, 0.0, 0.0};
    float move_speed = 5.0;
    bool rendering = false;
    bool quit = false;
    while (!quit) {
        // Render picture if move has changed
        if (rendering) {
            std::cout << "Rendering" << std::endl;

            // Move camera
            move.y = 0.0;
            if (move != vec3{0.0, 0.0, 0.0}) {
                scene.camera.origin += glm::normalize(move) * move_speed;
            }
            if (angle != vec3{0.0, 0.0, 0.0}) {
                scene.camera.dir += glm::normalize(angle);
            }

            render(pixels, scene);

            if (SDL_UpdateTexture(texture, NULL, pixels, WIDTH * sizeof(Uint32)) < 0) {
                std::cout << "ERROR: " << SDL_GetError() << std::endl;
            }

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            if (SDL_RenderCopy(renderer, texture, NULL, NULL) < 0) {
                std::cout << "ERROR: " << SDL_GetError() << std::endl;
            }
            SDL_RenderPresent(renderer);

            move.x = 0.0;
            move.y = 0.0;
            move.z = 0.0;
            angle.x = 0.0;
            angle.y = 0.0;
            angle.z = 0.0;
            rendering = false;
        }

        // Poll events
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_w:
                            move += glm::normalize(scene.camera.dir);
                            rendering = true;
                            break;
                        case SDLK_a:
                            move -= glm::normalize(scene.camera.rightVector());
                            rendering = true;
                            break;
                        case SDLK_s:
                            move -= glm::normalize(scene.camera.dir);
                            rendering = true;
                            break;
                        case SDLK_d:
                            move += glm::normalize(scene.camera.rightVector());
                            rendering = true;
                            break;
                        case SDLK_UP:
                            // angle.y += 1.0;
                            // rendering = true;
                            break;
                        case SDLK_LEFT:
                            angle.x -= 1.0;
                            rendering = true;
                            break;
                        case SDLK_DOWN:
                            // angle.y -= 1.0;
                            // rendering = true;
                            break;
                        case SDLK_RIGHT:
                            angle.x += 1.0;
                            rendering = true;
                            break;
                    }
                    break;
                case SDL_QUIT:
                    quit = true;
                    break;
            }
        }
    }

    delete[] pixels;
    delete[] buff;

    std::cout << "END" << std::endl;

    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}