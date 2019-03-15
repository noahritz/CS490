#ifndef __raytrace_HPP__
#define __raytrace_HPP__

#include <vector>
#include <SDL2/SDL.h>
#include <glm/vec3.hpp>
#include "rapidjson/document.h"

class Shape;

class Ray {
public:

    // Constructors
    Ray(); // Default
    Ray(const glm::vec3 o, const glm::vec3 v);

    glm::vec3 origin;
    glm::vec3 vector;

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
    void setFOV(float FOV);

};

class Light {
public:

    glm::vec3 position;
    glm::vec3 color;

    Light(glm::vec3 p, glm::vec3 c);

};


void render(Uint32 *buffer, int width, int height, rapidjson::Document &scene);

glm::vec3 trace(const Ray &r, int depth, std::vector<Shape*> objects);


#include "raytrace.cpp"

#endif