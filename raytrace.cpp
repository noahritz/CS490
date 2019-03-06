#include <glm/vec3.hpp>
#include <glm/trigonometric.hpp>
#include <glm/geometric.hpp>
#include "raytrace.hpp"
#include "geometry.hpp"
#include <iostream>

using glm::vec3;

/* RAY CLASS */
// Default Ray constructor
Ray::Ray() : origin{0.0, 0.0, 0.0}, vector{0.0, 0.0, 0.0} {};

// Ray constructor taking an origin and direction vector
Ray::Ray(const glm::vec3 o, const glm::vec3 v) : origin{o}, vector{v} {};

/* CAMERA CLASS */
// Default constructor for Camera
Camera::Camera() : origin{0.0, 0.0, 0.0}, dir{0.0, 0.0, -1.0} {
    setView(1, 1, 0.0);
}

Camera::Camera(int w, int h, float FOV) : origin{0.0, 0.0, 0.0}, dir{0.0, 0.0, -1.0} {
    setView(w, h, FOV);
}

// Set the field of view
void Camera::setView(int w, int h, float FOV) {
    width = w;
    height = h;
    aspectRatio = (float) height / (float) width;
    fov = FOV;
}

/* LIGHT CLASS */
Light::Light(glm::vec3 p, glm::vec3 c) : position{p}, color{c} {};

Uint32 vecToHex(glm::vec3 v) { // maybe inline this?
    return (((Uint32) (v.r * 255.0)) << 16) + (((Uint32) (v.g * 255.0)) << 8) + ((Uint32) (v.b * 255.0));
}

void render(Uint32 *buffer) {

    // Red sphere
    vec3 sphere{0.0, 0.0, -10.0};  
    vec3 sphere_color{1.0, 0.0, 0.0};
    float sphere_rad = 1.0;

    // Create a camera facing forward
    Camera camera{640, 480, 40.0};
    float fovRadians = glm::tan(camera.fov / 2 * (M_PI / 180)); // A misnomer, but whatever

    // Light
    vec3 light{3.0, 3.0, -8.0};
    vec3 light_color{1.0, 1.0, 1.0};

    Ray ray;
    
    for (int x = 0; x < 640; x++) {
        for (int y = 0; y < 480; y++) {
            
            // Start with a black pixel
            vec3 color{0.0, 0.0, 0.0};

            // Create ray
            ray.origin = camera.origin;

            float px = (2 * ((x + 0.5) / camera.width) - 1) * fovRadians * camera.aspectRatio;
            float py = (1 - 2 * ((y + 0.5) / camera.height)) * fovRadians;
            ray.vector = glm::normalize(vec3{px, py, -1});

            // Check for collisions with the scene
            color += trace(ray, 0);

            buffer[y*camera.width + x] = vecToHex(color);
        }
    }
}

vec3 trace(const Ray &r, int depth) {
    // Create a sphere
    Sphere s1{vec3{0.0, 0.0, -10.0}, 1.0};
    s1.color = vec3{1.0, 0.0, 0.0};

    // Light source
    Light light{vec3{2.0, 2.0, -8.0}, vec3{0.0, 0.0, 1.0}};

    // Intersect sphere
    float t;
    if (s1.intersect(r, t)) {
        glm::vec3 pHit = r.origin + (r.vector * t);

        // get surface details of intersection
        return s1.surface(pHit, light);
    }

    return vec3{0.0, 0.0, 0.0};
}
