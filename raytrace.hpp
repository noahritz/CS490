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

    float fov;
    const int WIDTH;
    const int HEIGHT;
    float aspectRatio;
    glm::vec3 origin;
    glm::vec3 dir;

    // Constructor
    Camera();
    Camera(int w, int h, float FOV);

    void setView(float FOV);
    void move(const glm::vec3 pos, const glm::vec3 point);

};

class Light {
public:

    glm::vec3 position;
    glm::vec3 color;

    Light(glm::vec3 p, glm::vec3 c);

    bool visible(const glm::vec3& point, const std::vector<Shape*>& objects) const;

};


void render(Uint32 *buffer, int width, int height, rapidjson::Document &scene);

glm::vec3 trace(const Ray &r, const std::vector<Shape*>& objects, const std::vector<Light*>& lights);

void tonemap(Uint32 *buffer, int w, int h);

#include "raytrace.cpp"

#endif