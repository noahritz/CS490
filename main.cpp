#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <assert.h>

#include <SDL2/SDL.h>
#include <glm/vec3.hpp>
#include <glm/common.hpp>
#include "rapidjson/document.h"

#include "raytrace.hpp"
#include "loader.hpp"

// #define DEBUG 1

const int PREVIEW_WIDTH = 160, PREVIEW_HEIGHT = 120;
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
    SDL_Texture *previewTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, PREVIEW_WIDTH, PREVIEW_HEIGHT);

    std::cout << "Controls:" << std::endl;
    std::cout << "Movement:\t\t\tWASD" << std::endl;
    std::cout << "Look Around:\t\t\tArrow Keys" << std::endl;
    std::cout << "Render Hi-Resolution:\t\tSPACE" << std::endl;
    std::cout << "Enter Password:\t\t\tENTER" << std::endl;

    // Read scene file from json
    int file_length = 0;
    char *buff;
    std::ifstream f;
    if (argc > 1) {
        f = std::ifstream(strcat(argv[1], "/scene.json"), std::ifstream::binary | std::ios::ate); // Read file from end
    } else {
        f = std::ifstream("scene.json", std::ifstream::binary | std::ios::ate); // Read file from end
    }

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
    #ifdef DEBUG
    std::cout << "Reading scene.json with length " << file_length << std::endl;
    #endif

    rapidjson::Document d;
    d.Parse(buff);
    assert(d.IsObject());

    /* Create scene */
    std::string password = d["password"].GetString();

    int num_objects = d["objects"]["spheres"].Size() + d["objects"]["triangles"].Size() + (d["objects"]["rectangles"].Size()*2) + (d["objects"]["texturedRectangles"].Size()*2);
    int num_lights = d["lights"].Size();
    int fov = d["camera"]["fov"].GetFloat();
    Scene scene{WIDTH, HEIGHT, fov, num_objects, num_lights};

    scene.camera.WIDTH = WIDTH;
    scene.camera.HEIGHT = HEIGHT;

    scene.camera.FULL_WIDTH = WIDTH;
    scene.camera.FULL_HEIGHT = HEIGHT;
    scene.camera.PREVIEW_WIDTH = PREVIEW_WIDTH;
    scene.camera.PREVIEW_HEIGHT = PREVIEW_HEIGHT;

    scene.camera.using_sprite = d["camera"]["sprite"].GetBool();

    // Create a camera facing forward
    scene.camera.fullHalfWidth = glm::tan((scene.camera.fov / 2) * (M_PI / 180)); // A misnomer, but whatever
    scene.camera.fullHalfHeight = scene.camera.fullHalfWidth * (WIDTH/HEIGHT);
    scene.camera.fullPixelWidth = (scene.camera.fullHalfWidth * 2) / (WIDTH - 1);
    scene.camera.fullPixelHeight = (scene.camera.fullHalfHeight * 2) / (HEIGHT - 1);

    scene.camera.previewHalfWidth = glm::tan((scene.camera.fov / 2) * (M_PI / 180)); // A misnomer, but whatever
    scene.camera.previewHalfHeight = scene.camera.previewHalfWidth * (PREVIEW_WIDTH/PREVIEW_HEIGHT);
    scene.camera.previewPixelWidth = (scene.camera.previewHalfWidth * 2) / (PREVIEW_WIDTH - 1);
    scene.camera.previewPixelHeight = (scene.camera.previewHalfHeight * 2) / (PREVIEW_HEIGHT - 1);

    scene.camera.setPreview(false);

    // Move camera
    vec3 cameraPos{d["camera"]["x"].GetFloat(), d["camera"]["y"].GetFloat(), d["camera"]["z"].GetFloat()};
    vec3 cameraPoint{d["camera"]["toX"].GetFloat(), d["camera"]["toY"].GetFloat(), d["camera"]["toZ"].GetFloat()};
    scene.camera.move(cameraPos, cameraPoint);

    // Create vector of scene objects and find scene bounding box
    glm::vec3 scene_min{10000.0, 10000.0, 10000.0};
    glm::vec3 scene_max{-10000.0, -10000.0, -10000.0};

    // Get sphere objects from json document
    int i = 0;
    glm::vec3 ctr;
    float rad;
    for (auto &s : d["objects"]["spheres"].GetArray()) {
        ctr = vec3{s["x"].GetFloat(), s["y"].GetFloat(), s["z"].GetFloat()};
        rad = s["radius"].GetFloat();
        Sphere *sph = new Sphere{   ctr, rad,
                                    vec3{s["r"].GetFloat(), s["g"].GetFloat(), s["b"].GetFloat()},
                                    d["materials"][s["material"].GetInt()]["lambert"].GetFloat(),
                                    d["materials"][s["material"].GetInt()]["specular"].GetFloat(),
                                    d["materials"][s["material"].GetInt()]["refractive"].GetBool(),
                                    d["materials"][s["material"].GetInt()]["IoR"].GetFloat()};
        scene.objects[i++] = sph;

        scene_min.x = std::min(ctr.x - rad, scene_min.x);
        scene_min.y = std::min(ctr.y - rad, scene_min.y);
        scene_min.z = std::min(ctr.z - rad, scene_min.z);
        scene_max.x = std::max(ctr.x + rad, scene_max.x);
        scene_max.y = std::max(ctr.y + rad, scene_max.y);
        scene_max.z = std::max(ctr.z + rad, scene_max.z);
    }

    // Get triangle objects from json document
    glm::vec3 p1;
    glm::vec3 p2;
    glm::vec3 p3;
    for (auto &t : d["objects"]["triangles"].GetArray()) {
        p1 = vec3{t["v1"]["x"].GetFloat(), t["v1"]["y"].GetFloat(), t["v1"]["z"].GetFloat()};
        p2 = vec3{t["v2"]["x"].GetFloat(), t["v2"]["y"].GetFloat(), t["v2"]["z"].GetFloat()};
        p3 = vec3{t["v3"]["x"].GetFloat(), t["v3"]["y"].GetFloat(), t["v3"]["z"].GetFloat()};
        Triangle *tri = new Triangle{   p1, p2, p3,
                                        vec3{t["r"].GetFloat(), t["g"].GetFloat(), t["b"].GetFloat()},
                                        d["materials"][t["material"].GetInt()]["lambert"].GetFloat(),
                                        d["materials"][t["material"].GetInt()]["specular"].GetFloat(),
                                        d["materials"][t["material"].GetInt()]["refractive"].GetBool(),
                                        d["materials"][t["material"].GetInt()]["IoR"].GetFloat()};
        scene.objects[i++] = tri;

        scene_min.x = std::min({p1.x, p2.x, p3.x, scene_min.x});
        scene_min.y = std::min({p1.y, p2.y, p3.y, scene_min.y});
        scene_min.z = std::min({p1.z, p2.z, p3.z, scene_min.z});
        scene_max.x = std::max({p1.x, p2.x, p3.x, scene_max.x});
        scene_max.y = std::max({p1.y, p2.y, p3.y, scene_max.y});
        scene_max.z = std::max({p1.z, p2.z, p3.z, scene_max.z});
    }

    // Create rectangles (an easier way to place geometry)
    for (auto &t : d["objects"]["rectangles"].GetArray()) {
        // Create Triangle 1
        p1 = vec3{t["bottomright"]["x"].GetFloat(), t["bottomright"]["y"].GetFloat(), t["bottomright"]["z"].GetFloat()};
        p2 = vec3{t["topleft"]["x"].GetFloat(), t["topleft"]["y"].GetFloat(), t["topleft"]["z"].GetFloat()};
        p3 = vec3{t["bottomleft"]["x"].GetFloat(), t["bottomleft"]["y"].GetFloat(), t["bottomleft"]["z"].GetFloat()};
        Triangle *tri1 = new Triangle{   p1, p2, p3,
                                        vec3{t["r"].GetFloat(), t["g"].GetFloat(), t["b"].GetFloat()},
                                        d["materials"][t["material"].GetInt()]["lambert"].GetFloat(),
                                        d["materials"][t["material"].GetInt()]["specular"].GetFloat(),
                                        d["materials"][t["material"].GetInt()]["refractive"].GetBool(),
                                        d["materials"][t["material"].GetInt()]["IoR"].GetFloat()};
        scene.objects[i++] = tri1;

        scene_min.x = std::min({p1.x, p2.x, p3.x, scene_min.x});
        scene_min.y = std::min({p1.y, p2.y, p3.y, scene_min.y});
        scene_min.z = std::min({p1.z, p2.z, p3.z, scene_min.z});
        scene_max.x = std::max({p1.x, p2.x, p3.x, scene_max.x});
        scene_max.y = std::max({p1.y, p2.y, p3.y, scene_max.y});
        scene_max.z = std::max({p1.z, p2.z, p3.z, scene_max.z});

        // Create Triangle 2
        p1 = vec3{t["bottomright"]["x"].GetFloat(), t["bottomright"]["y"].GetFloat(), t["bottomright"]["z"].GetFloat()};
        p2 = vec3{t["topright"]["x"].GetFloat(), t["topright"]["y"].GetFloat(), t["topright"]["z"].GetFloat()};
        p3 = vec3{t["topleft"]["x"].GetFloat(), t["topleft"]["y"].GetFloat(), t["topleft"]["z"].GetFloat()};
        Triangle *tri2 = new Triangle{   p1, p2, p3,
                                        vec3{t["r"].GetFloat(), t["g"].GetFloat(), t["b"].GetFloat()},
                                        d["materials"][t["material"].GetInt()]["lambert"].GetFloat(),
                                        d["materials"][t["material"].GetInt()]["specular"].GetFloat(),
                                        d["materials"][t["material"].GetInt()]["refractive"].GetBool(),
                                        d["materials"][t["material"].GetInt()]["IoR"].GetFloat()};
        scene.objects[i++] = tri2;

        scene_min.x = std::min({p1.x, p2.x, p3.x, scene_min.x});
        scene_min.y = std::min({p1.y, p2.y, p3.y, scene_min.y});
        scene_min.z = std::min({p1.z, p2.z, p3.z, scene_min.z});
        scene_max.x = std::max({p1.x, p2.x, p3.x, scene_max.x});
        scene_max.y = std::max({p1.y, p2.y, p3.y, scene_max.y});
        scene_max.z = std::max({p1.z, p2.z, p3.z, scene_max.z});
    }

    // Load Textures
    for (auto &t : d["textures"].GetArray()) {
        scene.textures.push_back(cimg_library::CImg<float>(t.GetString()));
    }

    // Load Camera sprite texture
    scene.textures.push_back(cimg_library::CImg<float>("textures/robot.bmp"));


    // Create textured rectangles
    for (auto &t : d["objects"]["texturedRectangles"].GetArray()) {
        // Create Triangle 1
        p1 = vec3{t["bottomright"]["x"].GetFloat(), t["bottomright"]["y"].GetFloat(), t["bottomright"]["z"].GetFloat()};
        p2 = vec3{t["topleft"]["x"].GetFloat(), t["topleft"]["y"].GetFloat(), t["topleft"]["z"].GetFloat()};
        p3 = vec3{t["bottomleft"]["x"].GetFloat(), t["bottomleft"]["y"].GetFloat(), t["bottomleft"]["z"].GetFloat()};
        TexturedTriangle *tri1 = new TexturedTriangle{   p1, p2, p3,
                                        d["materials"][t["material"].GetInt()]["lambert"].GetFloat(),
                                        d["materials"][t["material"].GetInt()]["specular"].GetFloat(),
                                        d["materials"][t["material"].GetInt()]["refractive"].GetBool(),
                                        d["materials"][t["material"].GetInt()]["IoR"].GetFloat(),
                                        scene.textures[t["texture"].GetInt()], true};
        scene.objects[i++] = tri1;

        scene_min.x = std::min({p1.x, p2.x, p3.x, scene_min.x});
        scene_min.y = std::min({p1.y, p2.y, p3.y, scene_min.y});
        scene_min.z = std::min({p1.z, p2.z, p3.z, scene_min.z});
        scene_max.x = std::max({p1.x, p2.x, p3.x, scene_max.x});
        scene_max.y = std::max({p1.y, p2.y, p3.y, scene_max.y});
        scene_max.z = std::max({p1.z, p2.z, p3.z, scene_max.z});

        // Create Triangle 2
        p1 = vec3{t["bottomright"]["x"].GetFloat(), t["bottomright"]["y"].GetFloat(), t["bottomright"]["z"].GetFloat()};
        p2 = vec3{t["topright"]["x"].GetFloat(), t["topright"]["y"].GetFloat(), t["topright"]["z"].GetFloat()};
        p3 = vec3{t["topleft"]["x"].GetFloat(), t["topleft"]["y"].GetFloat(), t["topleft"]["z"].GetFloat()};
        TexturedTriangle *tri2 = new TexturedTriangle{   p1, p2, p3,
                                        d["materials"][t["material"].GetInt()]["lambert"].GetFloat(),
                                        d["materials"][t["material"].GetInt()]["specular"].GetFloat(),
                                        d["materials"][t["material"].GetInt()]["refractive"].GetBool(),
                                        d["materials"][t["material"].GetInt()]["IoR"].GetFloat(),
                                        scene.textures[t["texture"].GetInt()], false};
        scene.objects[i++] = tri2;

        scene_min.x = std::min({p1.x, p2.x, p3.x, scene_min.x});
        scene_min.y = std::min({p1.y, p2.y, p3.y, scene_min.y});
        scene_min.z = std::min({p1.z, p2.z, p3.z, scene_min.z});
        scene_max.x = std::max({p1.x, p2.x, p3.x, scene_max.x});
        scene_max.y = std::max({p1.y, p2.y, p3.y, scene_max.y});
        scene_max.z = std::max({p1.z, p2.z, p3.z, scene_max.z});
    }

    // Get object models from json document
    glm::vec3 model_color, model_location;
    for (auto &m : d["objects"]["models"].GetArray()) {
        model_color = vec3{m["r"].GetFloat(), m["g"].GetFloat(), m["b"].GetFloat()};
        model_location = vec3{m["x"].GetFloat(), m["y"].GetFloat(), m["z"].GetFloat()};
        load(   scene.objects, m["filename"].GetString(),
                m["scale"].GetFloat(),
                model_location,
                model_color,
                d["materials"][m["material"].GetInt()]["lambert"].GetFloat(),
                d["materials"][m["material"].GetInt()]["specular"].GetFloat(),
                d["materials"][m["material"].GetInt()]["refractive"].GetBool(),
                d["materials"][m["material"].GetInt()]["IoR"].GetFloat());

    }

    // Create camera sprite billboard
    if (scene.camera.using_sprite) {
        // Create Triangle 1
        glm::vec3 right = scene.camera.rightVector();
        glm::vec3 up = scene.camera.upVector(right);
        //// Bottom Right
        p1 = scene.camera.origin + (2.0f*right) + (2.0f*up) - (0.1f * scene.camera.dir);
        //// Top Left
        p2 = scene.camera.origin - (2.0f*right) - (2.0f*up) - (0.1f * scene.camera.dir);
        //// Bottom Left
        p3 = scene.camera.origin - (2.0f*right) + (2.0f*up) - (0.1f * scene.camera.dir);
        TexturedTriangle *tri1 = new TexturedTriangle{p1, p2, p3, 1.0, 0.0, false, 1.0, scene.textures[scene.textures.size() - 1], true};
        scene.objects.push_back(tri1);

        // Create Triangle 2
        //// Bottom Right
        p1 = scene.camera.origin + (2.0f*right) + (2.0f*up) - (0.1f * scene.camera.dir);
        //// Top Right
        p2 = scene.camera.origin + (2.0f*right) - (2.0f*up) - (0.1f * scene.camera.dir);
        //// Top Left
        p3 = scene.camera.origin - (2.0f*right) - (2.0f*up) - (0.1f * scene.camera.dir);
        TexturedTriangle *tri2 = new TexturedTriangle{p1, p2, p3, 1.0, 0.0, false, 1.0, scene.textures[scene.textures.size() - 1], false};

        scene.objects.push_back(tri2);

        scene.camera.sprite_top = tri1;
        scene.camera.sprite_bottom = tri2;
    }

    // Get scene lights from json document
    i = 0;
    for (auto &l : d["lights"].GetArray()) {
        Light *lgt = new Light{ vec3{l["x"].GetFloat(), l["y"].GetFloat(), l["z"].GetFloat()},
                                vec3{l["r"].GetFloat(), l["g"].GetFloat(), l["b"].GetFloat()}};
        scene.lights[i++] = lgt;
    }

    // Create grid
    glm::vec3 grid_size{100.0, 100.0, 100.0};
    glm::ivec3 dimensions{d["grid"]["x"].GetInt(), d["grid"]["y"].GetInt(), d["grid"]["z"].GetInt()};
    // glm::ivec3 dimensions{5, 2, 5};
    glm::vec3 min{0.0, 0.0, 0.0};
    glm::vec3 max{100.0, 100.0, 100.0};
    Grid grid{grid_size, dimensions, min, max};

    /*  * NOTE: Automated grid creation (below) was producing inefficient
        * results. User designed grid sizes were more reliable
        
    float delta = 0.25;
    glm::vec3 grid_size = scene_max - scene_min;
    float grid_conversion = glm::pow((delta * scene.objects.size()) / (grid_size.x * grid_size.y * grid_size.z), 1.0/3.0);
    Grid grid{grid_size, (glm::ivec3) glm::round(grid_size * grid_conversion), scene_min, scene_max};
    for (int i = 0; i < 3; i++) {
        if (grid.dimensions[i] <= 0) {
            grid.dimensions[i] = 1;
        }
    }

    */
    
    // Fill grid with triangles
    int xx, yy, zz;
    glm::vec3 obj_min, obj_max;
    glm::ivec3 cell_min, cell_max;
    glm::vec3 fdimensions{(float) grid.dimensions.x, (float) grid.dimensions.y, (float) grid.dimensions.z};
    glm::vec3 cell_size = grid.size / fdimensions;
    int objs_placed = 0; // for testing
    for (auto o : scene.objects) {
        bool placed = false; // for testing

        obj_min = o->min();
        obj_max = o->max();
        cell_min = (glm::ivec3) glm::floor(obj_min / cell_size);
        cell_max = (glm::ivec3) glm::floor(obj_max / cell_size);

        if (o == scene.camera.sprite_top || o == scene.camera.sprite_bottom) {
            cell_min = {0, 0, 0};
            cell_max = dimensions - 1;
        }

        // Ensure objects on the end are placed in the grid
        for (int i = 0; i < 3; i++) {
            cell_min[i] = glm::clamp(cell_min[i], 0, grid.dimensions[i] - 1);
            cell_max[i] = glm::clamp(cell_max[i], 0, grid.dimensions[i] - 1);
        }

        for (xx = cell_min.x; xx <= cell_max.x && xx < grid.dimensions.x; xx++) {
            for (yy = cell_min.y; yy <= cell_max.y && yy < grid.dimensions.y; yy++) {
                for (zz = cell_min.z; zz <= cell_max.z && zz < grid.dimensions.z; zz++) {
                    grid.at(xx, yy, zz).push_back(o); // NOTE: Possible error point (floats converting to ints incorrectly, not entirely sure)
                    placed = true; // for testing
                }
            }
        }

        if (placed) {
            objs_placed++; // for testing
        }
    }

    // Test Uniform Grid Creation
    #ifdef DEBUG
    printf("Created %ix%ix%i uniform grid\n", grid.dimensions.x, grid.dimensions.y, grid.dimensions.z);
    std::cout << objs_placed << " objects placed into grid" << std::endl;
    #endif

    // Anti-Aliasing
    scene.AA = d["AA"].GetInt();

    // Pixel buffer
    Uint32 *pixels = new Uint32[WIDTH*HEIGHT];
    Uint32 *previewPixels = new Uint32[PREVIEW_WIDTH*PREVIEW_HEIGHT];

    // Set up initial camera angle
    float camera_theta = glm::atan(scene.camera.dir.z, scene.camera.dir.x);
    float camera_phi = glm::atan(glm::sqrt((scene.camera.dir.x*scene.camera.dir.x) + (scene.camera.dir.z*scene.camera.dir.z))/ scene.camera.dir.y);
    scene.camera.setAngle(camera_theta, camera_phi);

    // Render initial scene preview
    scene.camera.setPreview(true);
    render(previewPixels, scene, grid);

    if (SDL_UpdateTexture(previewTexture, NULL, previewPixels, PREVIEW_WIDTH * sizeof(Uint32)) < 0) {
        std::cout << "ERROR: " << SDL_GetError() << std::endl;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    if (SDL_RenderCopy(renderer, previewTexture, NULL, NULL) < 0) {
        std::cout << "ERROR: " << SDL_GetError() << std::endl;
    }
    SDL_RenderPresent(renderer);

    SDL_Event event;

    glm::vec3 move{0.0, 0.0, 0.0};
    float min_phi = M_PI/4;
    float max_phi = 6 * M_PI/8;
    float move_speed = 10.0;
    bool moving_vertical = false;
    bool rendering = false;
    bool rendering_preview = true;
    int AA = scene.AA;
    bool quit = false;
    while (!quit) {
        // Render picture if move has changed
        if (rendering) {


            // Move camera
            if (!moving_vertical) move.y = 0.0;
            scene.camera.translate(move * move_speed);

            camera_phi = glm::clamp(camera_phi, min_phi, max_phi);
            scene.camera.setAngle(camera_theta, camera_phi);

            #ifdef DEBUG
            std::cout << "Rendering" << std::endl;
            std::cout << "theta: " << camera_theta * 180.0 / M_PI << ", phi: " << camera_phi * 180.0 / M_PI << std::endl;
            #endif

            if (scene.camera.using_sprite) {
                // Position camera billboard sprite
                glm::vec3 right = scene.camera.rightVector();
                glm::vec3 up = scene.camera.upVector(right);

                //// Bottom Right
                p1 = scene.camera.origin + (2.0f*right) + (2.0f*up) - (0.01f * scene.camera.dir);
                //// Top Left
                p2 = scene.camera.origin - (2.0f*right) - (2.0f*up) - (0.01f * scene.camera.dir);
                //// Bottom Left
                p3 = scene.camera.origin - (2.0f*right) + (2.0f*up) - (0.01f * scene.camera.dir);
                scene.camera.sprite_top->v0 = p1;
                scene.camera.sprite_top->v1 = p2;
                scene.camera.sprite_top->v2 = p3;

                //// Bottom Right
                p1 = scene.camera.origin + (2.0f*right) + (2.0f*up) - (0.01f * scene.camera.dir);
                //// Top Right
                p2 = scene.camera.origin + (2.0f*right) - (2.0f*up) - (0.01f * scene.camera.dir);
                //// Top Left
                p3 = scene.camera.origin - (2.0f*right) - (2.0f*up) - (0.01f * scene.camera.dir);
                scene.camera.sprite_bottom->v0 = p1;
                scene.camera.sprite_bottom->v1 = p2;
                scene.camera.sprite_bottom->v2 = p3;
            }

            scene.camera.setPreview(rendering_preview);
            if (rendering_preview) {
                scene.AA = 1;
                render(previewPixels, scene, grid);
                if (SDL_UpdateTexture(previewTexture, NULL, previewPixels, PREVIEW_WIDTH * sizeof(Uint32)) < 0) {
                    std::cout << "ERROR: " << SDL_GetError() << std::endl;
                }
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);
                if (SDL_RenderCopy(renderer, previewTexture, NULL, NULL) < 0) {
                    std::cout << "ERROR: " << SDL_GetError() << std::endl;
                }
            } else {
                // Draw red outline while scene is rendering
                redOutline(previewPixels, scene.camera.PREVIEW_WIDTH, scene.camera.PREVIEW_HEIGHT, 1);
                if (SDL_UpdateTexture(previewTexture, NULL, previewPixels, PREVIEW_WIDTH * sizeof(Uint32)) < 0) {
                    std::cout << "ERROR: " << SDL_GetError() << std::endl;
                }
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);
                if (SDL_RenderCopy(renderer, previewTexture, NULL, NULL) < 0) {
                    std::cout << "ERROR: " << SDL_GetError() << std::endl;
                }
                SDL_RenderPresent(renderer);

                // Render detailed scene
                scene.AA = AA;
                render(pixels, scene, grid);
                if (SDL_UpdateTexture(texture, NULL, pixels, WIDTH * sizeof(Uint32)) < 0) {
                    std::cout << "ERROR: " << SDL_GetError() << std::endl;
                }
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);
                if (SDL_RenderCopy(renderer, texture, NULL, NULL) < 0) {
                    std::cout << "ERROR: " << SDL_GetError() << std::endl;
                }
            }

            SDL_RenderPresent(renderer);

            move.x = 0.0;
            move.y = 0.0;
            move.z = 0.0;
            moving_vertical = false;
            rendering = false;
            rendering_preview = true;
        }

        // Poll events
        while (SDL_PollEvent(&event) && !rendering) {
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
                        case SDLK_q:
                            // move += glm::normalize(scene.camera.upVector(scene.camera.rightVector()));
                            // moving_vertical = true;
                            // rendering = true;
                            break;
                        case SDLK_e:
                            // move -= glm::normalize(scene.camera.upVector(scene.camera.rightVector()));
                            // moving_vertical = true;
                            // rendering = true;
                            break;
                        case SDLK_UP:
                            camera_phi -= M_PI/16.0;
                            rendering = true;
                            break;
                        case SDLK_LEFT:
                            camera_theta -= M_PI/16.0;
                            rendering = true;
                            break;
                        case SDLK_DOWN:
                            camera_phi += M_PI/16.0;
                            rendering = true;
                            break;
                        case SDLK_RIGHT:
                            camera_theta += M_PI/16.0;
                            rendering = true;
                            break;
                        case SDLK_SPACE:
                            rendering = true;
                            rendering_preview = false;
                            break;
                        case SDLK_RETURN:
                            std::cout << "ENTER PASSWORD (ENTER to Confirm, ESC to Cancel): ";
                            char c;
                            std::string attempt;
                            SDL_StartTextInput();
                            SDL_Event text_event;
                            bool done = false;
                            while (!done) {
                                if (SDL_PollEvent(&text_event)) {
                                    switch (text_event.type) {
                                        case SDL_TEXTINPUT:
                                            std::cout << text_event.text.text;
                                            attempt += text_event.text.text;
                                            break;
                                        case SDL_KEYDOWN:
                                            switch (text_event.key.keysym.sym) {
                                                case SDLK_ESCAPE:
                                                    std::cout << std::endl;
                                                    done = true;
                                                    break;
                                                case SDLK_RETURN:
                                                    if (attempt == password) {
                                                        std::cout << std::endl << "CORRECT" << std::endl;
                                                        quit = true;
                                                    } else {
                                                        std::cout << std::endl << "INCORRECT" << std::endl;
                                                    }
                                                    done = true;
                                                    break;
                                                case SDLK_BACKSPACE:
                                                    if (attempt.length() > 0) {
                                                        attempt.pop_back();
                                                        std::cout << '\b' << ' ' << '\b';
                                                    }
                                                    break;
                                            }
                                            break;
                                    }
                                }
                            }

                            SDL_StopTextInput();
                            break;
                    }
                    break;
                case SDL_QUIT:
                    quit = true;
                    break;
            }
        }
    }

    // Cleanup
    for (auto &o : scene.objects) {
        delete o;
    }

    delete[] pixels;
    delete[] buff;

    std::cout << "END" << std::endl;

    SDL_DestroyTexture(texture);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}