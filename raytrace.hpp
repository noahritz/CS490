#ifndef __raytrace_HPP__
#define __raytrace_HPP__

#include <vector>
#include <SDL2/SDL.h>
#include <glm/vec3.hpp>
#include "rapidjson/document.h"

class Shape;
class Intersection;

class Ray {
public:

    // Constructors
    Ray(); // Default
    Ray(const glm::vec3 o, const glm::vec3 v);

    glm::vec3 origin;
    glm::vec3 vector;
    int depth;

    Intersection intersectScene(const std::vector<Shape*>& objects) const;

};

class Intersection {
public:

    bool hit;
    Shape *obj;
    glm::vec3 point;

    Intersection() {hit = false;}
};

class Camera {
public:

    glm::vec3 origin;
    glm::vec3 dir;
    float fov;
    const int WIDTH;
    const int HEIGHT;
    float aspectRatio;
    float halfWidth;
    float halfHeight;
    float pixelHeight;
    float pixelWidth;

    // Constructor
    Camera();
    Camera(int w, int h, float FOV);

    void setView(float FOV);
    void move(const glm::vec3 pos, const glm::vec3 point);
    void translate(const glm::vec3 move_by);
    void setAngle(const float theta, const float phi);
    glm::vec3 rightVector() const;
    glm::vec3 upVector(glm::vec3 &right) const;

};

class Light {
public:

    glm::vec3 position;
    glm::vec3 color;

    Light(glm::vec3 p, glm::vec3 c);

    bool visible(const glm::vec3& point, const std::vector<Shape*>& objects) const;

};

class Scene {
public:

    Camera camera;
    std::vector<Shape*> objects;
    std::vector<Light*> lights;
    int AA;

    Scene(int w, int h, float fov, int total_objects, int total_lights);

};

class Grid {
public:

    glm::vec3 size;
    glm::ivec3 dimensions;
    std::vector<std::vector<Shape *>> cells;
    glm::vec3 min;
    glm::vec3 max;

    Grid(glm::vec3 s, glm::ivec3 dim, glm::vec3 grid_min, glm::vec3 grid_max);

    std::vector<Shape *>& at(int x, int y, int z);
};

void render(Uint32 *buffer, rapidjson::Document &scene, Grid& grid);

glm::vec3 trace(const Ray &r, const std::vector<Shape*>& objects, const std::vector<Light*>& lights);

void fillBuffer(Uint32 *buffer, std::vector<glm::vec3> pixels, int size);

#include "raytrace.cpp"

#endif