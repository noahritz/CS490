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
Ray::Ray() : origin{0.0, 0.0, 0.0}, vector{0.0, 0.0, 0.0}, depth(0) {};

// Ray constructor taking an origin and direction vector
Ray::Ray(const glm::vec3 o, const glm::vec3 v) : origin{o}, vector{v}, depth(0) {};

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
        }
    }

    return collision;
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

Uint32 vecToHex(glm::vec3 v) { // maybe inline this?
    return (((Uint32) (v.r * 255.0)) << 16) + (((Uint32) (v.g * 255.0)) << 8) + ((Uint32) (v.b * 255.0));
}

void hexToVec(glm::vec3 &v, const Uint32 &h) {
    v.r = ((h >> 16) & 255) / 255.0;
    v.g = ((h >> 8) & 255) / 255.0;
    v.b = (h & 255) / 255.0;
}

void render(Uint32 *buffer, int width, int height, rapidjson::Document &scene) {

    // Red sphere
    vec3 sphere{0.0, 0.0, -10.0};  
    vec3 sphere_color{1.0, 0.0, 0.0};
    float sphere_rad = 1.0;

    // Create a camera facing forward
    Camera camera{width, height, scene["camera"]["fov"].GetFloat()};
    float halfWidth = glm::tan((camera.fov / 2) * (M_PI / 180)); // A misnomer, but whatever
    float halfHeight = halfWidth * (width/height);
    float pixelWidth = (halfWidth * 2) / (width - 1);
    float pixelHeight = (halfHeight * 2) / (height - 1);

    // Move camera
    vec3 cameraPos{scene["camera"]["x"].GetFloat(), scene["camera"]["y"].GetFloat(), scene["camera"]["z"].GetFloat()};
    vec3 cameraPoint{scene["camera"]["toX"].GetFloat(), scene["camera"]["toY"].GetFloat(), scene["camera"]["toZ"].GetFloat()};
    camera.move(cameraPos, cameraPoint);

    // Establish camera direction
    vec3 cameraForward = glm::normalize(camera.dir - camera.origin);
    vec3 cameraRight = glm::normalize(glm::cross(cameraForward, vec3{0.0, 1.0, 0.0}));
    vec3 cameraUp = glm::normalize(glm::cross(cameraForward, cameraRight));
    vec3 px, py;


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
                                    s["radius"].GetFloat(),
                                    vec3{s["r"].GetFloat(), s["g"].GetFloat(), s["b"].GetFloat()},
                                    scene["materials"][s["material"].GetInt()]["lambert"].GetFloat(),
                                    scene["materials"][s["material"].GetInt()]["specular"].GetFloat()};
        objects[i++] = sph;
    }

    // Get triangle objects from json document
    for (auto &t : scene["objects"]["triangles"].GetArray()) {
        Triangle *tri = new Triangle{   vec3{t["v1"]["x"].GetFloat(), t["v1"]["y"].GetFloat(), t["v1"]["z"].GetFloat()},
                                        vec3{t["v2"]["x"].GetFloat(), t["v2"]["y"].GetFloat(), t["v2"]["z"].GetFloat()},
                                        vec3{t["v3"]["x"].GetFloat(), t["v3"]["y"].GetFloat(), t["v3"]["z"].GetFloat()},
                                        vec3{t["r"].GetFloat(), t["g"].GetFloat(), t["b"].GetFloat()},
                                        scene["materials"][t["material"].GetInt()]["lambert"].GetFloat(),
                                        scene["materials"][t["material"].GetInt()]["specular"].GetFloat()};
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

    // Create viewing ray
    Ray ray;
    
    const int AA = scene["AA"].GetInt(); // Anti-Aliasing

    auto start = std::chrono::high_resolution_clock::now();
    auto recent = start;

    for (int x = 0; x < camera.WIDTH; x++) {
        for (int y = 0; y < camera.HEIGHT; y++) {
            
            // Start with a black pixel
            vec3 color{0.0, 0.0, 0.0};

            for (int xx = 1; xx <= AA; xx++) {
                for (int yy = 1; yy <= AA; yy++) {

                    // Create ray
                    ray.origin = camera.origin;

                    // float px = (2 * ((x + ( (float) xx / (float) (AA + 1)) ) / camera.WIDTH) - 1) * halfWidth * camera.aspectRatio;
                    // float py = (1 - 2 * ((y + ( (float) yy / (float) (AA + 1)) ) / camera.HEIGHT)) * halfWidth;

                    // ray.vector = glm::normalize(vec3{px, py, -1});

                    px = cameraRight * (( (x + (float) xx / (float) (AA + 1)) * pixelWidth) - halfWidth) * camera.aspectRatio;
                    py = cameraUp * (( (y + (float) yy / (float) (AA + 1)) * pixelHeight) - halfHeight);

                    ray.vector = glm::normalize(cameraForward + px + py);

                    // Check for collisions with the scene
                    color += trace(ray, objects, lights);

                }
            }

            buffer[y*camera.WIDTH + x] = vecToHex(color / (float) (AA*AA) );

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

    // Tone map
    tonemap(buffer, width, height);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end-start);
    std::cout << "Execution time: " << (double) duration.count() / 1000000.0 << " seconds" << std::endl;
}

vec3 trace(const Ray &ray, const vector<Shape*>& objects, const vector<Light*>& lights) {

    // Return black after 2 bounces
    if (ray.depth > 2) return vec3{0.0, 0.0, 0.0};

    // Intersect object(s)
    float t = 10000.0;
    float t_test;
    bool hit = false;
    Shape *hit_object;

    for (Shape *o : objects) {
        if (o->intersect(ray, t_test) && t_test < t) {
            t = t_test;
            hit = true;
            hit_object = o;
        }
    }

    Intersection collision = ray.intersectScene(objects);

    if (collision.hit) {
        // get surface details of intersection
        // return vec3{0.0, 0.5, 0.0};
        glm::vec3 pHit = ray.origin + (ray.vector * t);
        return hit_object->surface(ray, pHit, objects, lights);
    }

    return vec3{0.0, 0.0, 0.0};
}

void tonemap(Uint32 *buffer, int w, int h) {
    vec3 temp;
    int index;
    float maxI = 0.00001;
    float I;
    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            index = y*w + x;
            hexToVec(temp, buffer[index]);

            I = (0.3*temp.r) + (0.5*temp.g) + (0.2*temp.b);
            if (I > maxI) {
                maxI = I;
                // std::cout << '(' << temp.r << ", " << temp.g << ", " << temp.b << ')' << std::endl;
            }

            // temp.r /= (temp.r + 0.25);
            // temp.g /= (temp.g + 0.25);
            // temp.b /= (temp.b + 0.25);

        }
    }


    float mult = 1.0/maxI;
    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            index = y*w + x;
            hexToVec(temp, buffer[index]);

            // temp *= mult;
            // if (temp.r > 1.0) temp.r = 1.0;
            // if (temp.g > 1.0) temp.g = 1.0;
            // if (temp.b > 1.0) temp.b = 1.0;

            buffer[index] = vecToHex(temp);
        }
    }
}