#include <glm/vec3.hpp>
#include <glm/trigonometric.hpp>
#include <glm/geometric.hpp>
#include "raytrace.hpp"

using glm::vec3;

// Default Ray constructor
Ray::Ray() : origin{0.0, 0.0, 0.0}, vector{0.0, 0.0, 0.0} {};

// Ray constructor taking an origin and direction vector
Ray::Ray(const glm::vec3 o, const glm::vec3 v) : origin{o}, vector{v} {};

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

Uint32 vecToHex(glm::vec3 v) {
    return (((int) (v.r * 255)) << 16) + (((int) (v.g * 255)) << 8) + ((int) v.b * 255);
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

    // Define screen characterstics
    // double pixelWidth = 

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
            color += trace(ray, sphere, 0);

            buffer[y*camera.width + x] = vecToHex(color);
            if (x == 0 && y == 0) {
                std::cout << std::hex << vecToHex(color) << std::endl;
            }

        }
    }
}

vec3 trace(Ray r, glm::vec3 sphere, int depth) {
    // Red sphere
    vec3 sphere{0.0, 0.0, -10.0};  
    vec3 sphere_color{1.0, 0.0, 0.0};
    float sphere_rad = 1.0;

    return vec3{0.5, 0.0, 1.0};
}