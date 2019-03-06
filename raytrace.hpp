#ifndef __raytrace_HPP__
#define __raytrace_HPP__

#include <SDL2/SDL.h>
#include <glm/vec3.hpp>

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
    int width;
    int height;
    float aspectRatio;
    glm::vec3 origin;
    glm::vec3 dir;

    // Constructor
    Camera();
    Camera(int w, int h, float FOV);

    void setView(int w, int h, float FOV);
    void setFOV(float FOV);

};

class Light {
public:

    glm::vec3 position;
    glm::vec3 color;

    Light(glm::vec3 p, glm::vec3 c);

};


void render(Uint32 *buffer);

glm::vec3 trace(const Ray &r, int depth);


#include "raytrace.cpp"

#endif