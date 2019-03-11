#include "geometry.hpp"
#include <glm/geometric.hpp>
#include <glm/exponential.hpp>
#include <algorithm>

Sphere::Sphere(glm::vec3 c, float r) : center{c}, radius{r} {}

// from https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection
bool Sphere::solveQuadratic (const float &a, const float &b, const float &c, float &x0, float &x1) const {
    float discr = (b * b) - (4 * a * c);
    if (discr < 0) {
        return false;
    } else if (discr == 0) {
        x0 = x1 = -b/(2 * a);
    } else {
        float q = (b > 0) ? (-0.5 * (b + glm::sqrt(discr))) : (-0.5 * (b - glm::sqrt(discr)));
        x0 = q / a;
        x1 = c / q;
    }

    if (x0 > x1) {
        std::swap(x0, x1);
    }

    return true;
}

// from https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/minimal-ray-tracer-rendering-spheres
bool Sphere::intersect(const Ray &ray, float &t) const {
    float t0, t1;

    glm::vec3 L = ray.origin - center;
    float a = glm::dot(ray.vector, ray.vector);
    float b = 2 * glm::dot(ray.vector, L);
    float c = glm::dot(L, L) - (radius*radius);

    if (!solveQuadratic(a, b, c, t0, t1)) {
        return false;
    }

    if (t0 > t1) {
        std::swap(t0, t1);
    }

    if (t0 < 0) {
        if (t1 < 0) {
            return false;
        }
        t0 = t1;
    }

    t = t0;

    return true;
}

glm::vec3 Sphere::surface(const glm::vec3& point, const Light& light) const {
    glm::vec3 color{0.0, 0.0, 0.0};

    float contribution = glm::dot(glm::normalize(light.position - point), normal(point));
    if (contribution > 0) {
        color += (light.color * contribution);
        color *= this->color;
    }

    return color;
}

glm::vec3 Sphere::normal(const glm::vec3& point) const {
    return glm::normalize(point - center);
}