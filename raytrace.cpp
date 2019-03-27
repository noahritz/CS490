#include <iostream>
#include <vector>
#include <chrono>

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
Ray::Ray() : origin{0.0, 0.0, 0.0}, vector{0.0, 0.0, 0.0} {};

// Ray constructor taking an origin and direction vector
Ray::Ray(const glm::vec3 o, const glm::vec3 v) : origin{o}, vector{v} {};

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

/* LIGHT CLASS */
Light::Light(glm::vec3 p, glm::vec3 c) : position{p}, color{c} {};

Uint32 vecToHex(glm::vec3 v) { // maybe inline this?
    return (((Uint32) (v.r * 255.0)) << 16) + (((Uint32) (v.g * 255.0)) << 8) + ((Uint32) (v.b * 255.0));
}

void render(Uint32 *buffer, int width, int height, rapidjson::Document &scene) {

    // Red sphere
    vec3 sphere{0.0, 0.0, -10.0};  
    vec3 sphere_color{1.0, 0.0, 0.0};
    float sphere_rad = 1.0;

    // Create a camera facing forward
    Camera camera{width, height, 40.0};
    float fovRadians = glm::tan(camera.fov / 2 * (M_PI / 180)); // A misnomer, but whatever
    // camera.move(/*TODO*/);

    // Light
    vec3 light{3.0, 3.0, -8.0};
    vec3 light_color{1.0, 1.0, 1.0};

    // Create vector of scene objects
    int num_objects = scene["objects"]["spheres"].Size() + scene["objects"]["triangles"].Size();
    vector<Shape*> objects{num_objects};

    // Get sphere objects from json document
    int i = 0;
    for (auto &s : scene["objects"]["spheres"].GetArray()) {
        Sphere *sph = new Sphere{   vec3{s["x"].GetFloat(), s["y"].GetFloat(), s["z"].GetFloat()},
                                    vec3{s["r"].GetFloat(), s["g"].GetFloat(), s["b"].GetFloat()},
                                    s["radius"].GetFloat()};
        objects[i++] = sph;
    }

    // Get triangle objects from json document
    for (auto &t : scene["objects"]["triangles"].GetArray()) {
        Triangle *tri = new Triangle{   vec3{t["v1"]["x"].GetFloat(), t["v1"]["y"].GetFloat(), t["v1"]["z"].GetFloat()},
                                        vec3{t["v2"]["x"].GetFloat(), t["v2"]["y"].GetFloat(), t["v2"]["z"].GetFloat()},
                                        vec3{t["v3"]["x"].GetFloat(), t["v3"]["y"].GetFloat(), t["v3"]["z"].GetFloat()},
                                        vec3{t["r"].GetFloat(), t["g"].GetFloat(), t["b"].GetFloat()}};
        objects[i++] = tri;
    }

    // Get scene lights from json document
    int num_lights = scene["lights"].Size();
    vector<Light*> lights{num_lights};
    i = 0;
    for (auto &l : scene["lights"].GetArray()) {
        Light *lgt = new Light{ vec3{l["x"].GetFloat(), l["y"].GetFloat(), l["z"].GetFloat()},
                                vec3{l["r"].GetFloat(), l["g"].GetFloat(), l["b"].GetFloat()}};
        lights[i++] = lgt;
    }

    Ray ray;
    
    auto start = std::chrono::high_resolution_clock::now();

    for (int x = 0; x < camera.WIDTH; x++) {
        for (int y = 0; y < camera.HEIGHT; y++) {
            
            // Start with a black pixel
            vec3 color{0.0, 0.0, 0.0};

            // Create ray
            ray.origin = camera.origin;

            float px = (2 * ((x + 0.5) / camera.WIDTH) - 1) * fovRadians * camera.aspectRatio;
            float py = (1 - 2 * ((y + 0.5) / camera.HEIGHT)) * fovRadians;
            ray.vector = glm::normalize(vec3{px, py, -1});

            // Check for collisions with the scene
            color += trace(ray, 0, objects, lights);

            buffer[y*camera.WIDTH + x] = vecToHex(color);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end-start);
    std::cout << "Execution time: " << duration.count() << " microseconds" << std::endl;
}

vec3 trace(const Ray &r, int depth, const vector<Shape*> objects, const vector<Light*> lights) {

    // Intersect object(s)
    float t = 10000.0;
    float t_test;
    bool hit = false;
    Shape *hit_object;

    for (Shape *o : objects) {
        if (o->intersect(r, t_test) && t_test < t) {
            // std::cout << "intersected" << std::endl;
            t = t_test;
            hit = true;
            hit_object = o;
        }
    }

    if (hit) {
        // get surface details of intersection
        glm::vec3 pHit = r.origin + (r.vector * t);
        return hit_object->surface(pHit, *lights[0]);
    }

    // Hit nothing, return black
    return vec3{0.0, 0.0, 0.0};
}
