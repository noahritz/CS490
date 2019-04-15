#include <iostream>
#include <vector>
#include <chrono>
#include <omp.h>

#include <glm/vec3.hpp>
#include <glm/trigonometric.hpp>
#include <glm/geometric.hpp>
#include "rapidjson/document.h"

#include "raytrace.hpp"
#include "geometry.hpp"

using glm::vec3;
using std::vector;

/* RAY CLASS */
// Default Ray constructor
Ray::Ray() : origin{0.0, 0.0, 0.0}, vector{0.0, 0.0, 0.0}, depth(0) {
    invdir = 1.0f/vector;
};

// Ray constructor taking an origin and direction vector
Ray::Ray(const glm::vec3 o, const glm::vec3 v) : origin{o}, vector{v}, depth(0) {
    invdir = 1.0f/vector; 
};

// Intersect Ray with a scene
Intersection Ray::intersectScene(const std::vector<Shape*>& objects) const {
    Intersection collision;

    float t = 10000.0;
    float t_test;
    
    for (Shape *o : objects) {
        if (o->intersect(*this, t_test) && t_test < t) {
            t = t_test;
            collision.hit = true;
            collision.obj = o;
            collision.point = origin + (vector * t);
            collision.t = t;
        }
    }

    return collision;
}

// Adapted from https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
bool Ray::intersectBox(vec3 min, vec3 max, float &t_min, float &t_max) const {
    glm::vec3 sign{(invdir.x < 0), (invdir.y < 0), (invdir.z < 0)};
    glm::vec3 bounds[] = {min, max};

    float t_ymin, t_ymax, t_zmin, t_zmax;

    t_min = (bounds[(int) sign[0]].x - origin.x) * invdir.x;
    t_max = (bounds[(int) (1-sign[0])].x - origin.x) * invdir.x;

    t_ymin = (bounds[(int) sign[1]].y - origin.y) * invdir.y;
    t_ymax = (bounds[(int) (1-sign[1])].y - origin.y) * invdir.y;

    if ((t_min > t_ymax) || (t_ymin > t_max))
        return false;
    if (t_ymin > t_min)
        t_min = t_ymin;
    if (t_ymax < t_max)
        t_max = t_ymax;

    t_zmin = (bounds[(int) sign[2]].z - origin.z) * invdir.z;
    t_zmax = (bounds[(int) (1-sign[2])].z - origin.z) * invdir.z;

    if ((t_min > t_zmax) || (t_zmin > t_max))
        return false;
    if (t_zmin > t_min)
        t_min = t_zmin;
    if (t_zmax < t_max)
        t_max = t_zmax;

    return true; 
}

/* CAMERA CLASS */
// Default constructor for Camera
Camera::Camera() : WIDTH(640), HEIGHT(480), origin{0.0, 0.0, 0.0}, dir{0.0, 0.0, -1.0} {
    setView(30.0);
}

Camera::Camera(int w, int h, float FOV) : WIDTH(w), HEIGHT(h), origin{0.0, 0.0, 0.0}, dir{0.0, 0.0, -1.0} {
    setView(FOV);
}

// Set the field of view
void Camera::setView(float FOV) {
    fov = FOV;
    aspectRatio = (float) WIDTH / (float) HEIGHT;
}

// Move Camera
void Camera::move(const vec3 pos, const vec3 point) {
    origin = pos;
    dir = point;
}

// Move Camera by given amount
void Camera::translate(const glm::vec3 move_by) {
    if (move_by != vec3{0.0, 0.0, 0.0}) {
        origin += glm::normalize(move_by);
    }
}

// Set Camera Angle
void Camera::setAngle(const float theta, const float phi) {
    glm::vec3 angle = glm::normalize(vec3{glm::cos(theta) * glm::sin(phi), glm::cos(phi), glm::sin(theta) * glm::sin(phi)});
    if (angle == vec3{0.0, 0.0, 0.0}) {
        dir = angle;
    } else {
        dir = glm::normalize(angle);
    }
}

vec3 Camera::rightVector() const {
    return glm::normalize(glm::cross(glm::normalize(dir), vec3{0.0, 1.0, 0.0}));
}

vec3 Camera::upVector(vec3 &right) const {
    return glm::normalize(glm::cross(glm::normalize(dir), right));
}

/* LIGHT CLASS */
Light::Light(glm::vec3 p, glm::vec3 c) : position{p}, color{c} {};

bool Light::visible(const glm::vec3& point, const std::vector<Shape*>& objects) const {
    Ray light_ray = Ray{point, glm::normalize(position - point)};

    Intersection intersect = light_ray.intersectScene(objects);
    if (intersect.hit && glm::distance(point, intersect.point) < glm::distance(point, position)) {
        return false;
    }

    return true;
}

/* SCENE CLASS */
Scene::Scene(int w, int h, float fov, int total_objects, int total_lights): camera(Camera{w, h, fov}), objects(std::vector<Shape*>{total_objects}), lights(std::vector<Light*>{total_lights}) {}

/* GRID CLASS */
Grid::Grid(vec3 s, glm::ivec3 dim, glm::vec3 grid_min, glm::vec3 grid_max) :    size(s),
                                                                                dimensions(dim),
                                                                                min(grid_min),
                                                                                max(grid_max),
                                                                                cells(dim.x*dim.y*dim.z) {}

vector<Shape *>& Grid::at(int x, int y, int z) {
    return cells[(dimensions.x * dimensions.y * z) + (dimensions.x * y) + x];
}

Uint32 vecToHex(glm::vec3 v) { // maybe inline this?
    return (((Uint32) (v.r * 255.0)) << 16) + (((Uint32) (v.g * 255.0)) << 8) + ((Uint32) (v.b * 255.0));
}

void hexToVec(glm::vec3 &v, const Uint32 &h) {
    v.r = ((h >> 16) & 255) / 255.0;
    v.g = ((h >> 8) & 255) / 255.0;
    v.b = (h & 255) / 255.0;
}

void render(Uint32 *buffer, Scene &scene, Grid& grid) {

    // Create vector of rgb pixels
    std::vector<vec3> pixels(scene.camera.WIDTH*scene.camera.HEIGHT);

    // Establish camera direction
    vec3 cameraForward = glm::normalize(scene.camera.dir);
    vec3 cameraRight = scene.camera.rightVector();
    vec3 cameraUp = scene.camera.upVector(cameraRight);
    vec3 px, py;

    // Create viewing ray
    Ray ray;
    

    auto start = std::chrono::high_resolution_clock::now();
    auto recent = start;

    #pragma omp parallel 
    {
        int tid = omp_get_thread_num();
        std::cout << tid << std::endl;
    }

    #pragma omp parallel for shared(pixels, buffer) private(px, py, ray)
    for (int x = 0; x < scene.camera.WIDTH; x++) {
        for (int y = 0; y < scene.camera.HEIGHT; y++) {
            
            // Start with a black pixel
            vec3 color{0.0, 0.0, 0.0};

            for (int xx = 1; xx <= scene.AA; xx++) {
                for (int yy = 1; yy <= scene.AA; yy++) {

                    // Create ray
                    ray.origin = scene.camera.origin;

                    // float px = (2 * ((x + ( (float) xx / (float) (AA + 1)) ) / scene.camera.WIDTH) - 1) * halfWidth * scene.camera.aspectRatio;
                    // float py = (1 - 2 * ((y + ( (float) yy / (float) (AA + 1)) ) / scene.camera.HEIGHT)) * halfWidth;

                    // ray.vector = glm::normalize(vec3{px, py, -1});

                    px = cameraRight * (( (x + (float) xx / (float) (scene.AA + 1)) * scene.camera.pixelWidth) - scene.camera.halfWidth) * scene.camera.aspectRatio;
                    py = cameraUp * (( (y + (float) yy / (float) (scene.AA + 1)) * scene.camera.pixelHeight) - scene.camera.halfHeight);

                    ray.vector = glm::normalize(cameraForward + px + py);
                    ray.invdir = 1.0f/ray.vector;

                    // Check for collisions with the scene
                    color += trace(ray, scene.objects, scene.lights, grid);

                }
            }

            pixels[y*scene.camera.WIDTH + x] = color;
            buffer[y*scene.camera.WIDTH + x] = vecToHex(color / (float) (scene.AA*scene.AA) );

            // Check for events to prevent the window from not responding
            if (y == 0) {
                auto check = std::chrono::high_resolution_clock::now();
                if (std::chrono::duration_cast<std::chrono::microseconds>(check - recent).count() > 1000000) {
                    SDL_PumpEvents();
                    recent = check;
                }
            }
        }
    }

    // convert vec3 vector to a Uint32 array with tone mapping
    fillBuffer(buffer, pixels, scene.camera.WIDTH * scene.camera.HEIGHT);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end-start);
    std::cout << "Execution time: " << (double) duration.count() / 1000000.0 << " seconds" << std::endl;
}

vec3 trace(const Ray &ray, const vector<Shape*>& objects, const vector<Light*>& lights, Grid& grid) {

    // Return black after 2 bounces
    if (ray.depth > 4) return vec3{0.0, 0.0, 0.0};

    /* ** Traverse grid ** */
    // Check if ray intersects grid
    float t_min, t_max;
    if (!ray.intersectBox(grid.min, grid.max, t_min, t_max)) {
        return vec3{0.0, 0.0, 0.0};
    }

    // Setup traversal
    vec3 cell_dimensions = (grid.size) / (vec3) grid.dimensions;

    glm::vec3 ray_orig_cell, delta_t, next_crossing_t;
    glm::ivec3 step, exit, current_cell;
    for (int i = 0; i < 3; i++) {
        ray_orig_cell[i] = (ray.origin[i] + (ray.vector[i] * t_min)) - grid.min[i];
        current_cell[i] = glm::clamp((int) glm::floor(ray_orig_cell[i] / cell_dimensions[i]), 0, grid.dimensions[i] - 1);
        if (ray.vector[i] < 0) {
            delta_t[i] = -cell_dimensions[i] * ray.invdir[i];
            next_crossing_t[i] = t_min + (current_cell[i] * cell_dimensions[i] - ray_orig_cell[i]) * ray.invdir[i];
            exit[i] = -1;
            step[i] = -1;
        } else {
            delta_t[i] = cell_dimensions[i] * ray.invdir[i];
            next_crossing_t[i] = t_min + ((current_cell[i] + 1) * cell_dimensions[i] - ray_orig_cell[i]) * ray.invdir[i];
            exit[i] = grid.dimensions[i];
            step[i] = 1;
        }
    }

    // Traverse grid
    Intersection collision = ray.intersectScene(objects);
    while (true) {
        collision = ray.intersectScene(grid.at(current_cell.x, current_cell.y, current_cell.z));

        Uint8 k =   ((next_crossing_t.x < next_crossing_t.y) << 2) + 
                    ((next_crossing_t.x < next_crossing_t.z) << 1) + 
                    ((next_crossing_t.y < next_crossing_t.z));
        static const Uint8 map[8] = {2, 1, 2, 1, 2, 2, 0, 0};
        Uint8 axis = map[k];

        if (collision.t < next_crossing_t[axis])
            break;

        current_cell[axis] += step[axis];

        if (current_cell[axis] == exit[axis])
            break;

        next_crossing_t[axis] += delta_t[axis];
    }

    if (collision.hit) {
        // get surface details of intersection
        // return {0.1, 0.4, 0.1};
        return collision.obj->surface(ray, collision.point, objects, lights, grid);
    }

    return vec3{0.0, 0.0, 0.0};
}

void fillBuffer(Uint32 *buffer, std::vector<vec3> pixels, int size) {
    float maxI = 0.00001;
    float I;
    for (int i = 0; i < size; i++) {
        I = (0.3 * pixels[i].r) + (0.5 * pixels[i].g) + (0.2 * pixels[i].b);
        if (I > maxI) {
            maxI = I;
        }
    }

    float mult = 1.0/maxI;
    for (int i = 0; i < size; i++) {
        pixels[i] *= mult;
        if (pixels[i].r > 1.0) pixels[i].r = 1.0;
        if (pixels[i].g > 1.0) pixels[i].g = 1.0;
        if (pixels[i].b > 1.0) pixels[i].b = 1.0;

        buffer[i] = vecToHex(pixels[i]);
    }
}