#include <glm/geometric.hpp>
#include <glm/exponential.hpp>
#include <algorithm>
#include "geometry.hpp"

/* SHAPE */

Shape::Shape() : color(glm::vec3{1.0, 1.0, 1.0}) {}
Shape::Shape(glm::vec3 col) : color(col) {}

glm::vec3 Shape::surface(const glm::vec3& point, const Light& light) const {
    glm::vec3 _color{0.0, 0.0, 0.0};

    float contribution = glm::dot(glm::normalize(light.position - point), normal(point));
    if (contribution > 0) {
        _color += (light.color * contribution);
        _color *= this->color;
    }

    return _color;
}


/* SPHERE */

// Constructor for a white sphere
Sphere::Sphere(glm::vec3 ctr, float r) : center{ctr}, radius{r} {}

// Constructor for a sphere of specified color
Sphere::Sphere(glm::vec3 ctr, glm::vec3 col, float r) : Shape(col), center{ctr}, radius{r} {}

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

    // std::cout << "sphere hit" << std::endl;
    return true;
}

glm::vec3 Sphere::normal(const glm::vec3& point) const {
    return glm::normalize(point - center);
}


/* TRIANGLE */

Triangle::Triangle(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2) :
    v0(p0), v1(p1), v2(p2) {}
Triangle::Triangle(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 col) :
    Shape(col), v0(p0), v1(p1), v2(p2) {}

// From https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
bool Triangle::intersect(const Ray& ray, float &t) const {
    // TODO
    glm::vec3 AB = v1 - v0;
    glm::vec3 AC = v2 - v0;
    glm::vec3 cramer_p = glm::cross(ray.vector, AC);
    float det = glm::dot(AB, cramer_p);
    
    // Disregard triangle if triangle is backfacing
    if (det < 0.0000001) {
        // std::cout << "tri miss BACK " << det << std::endl;
        return false;
    }

    float inv_det = 1.0 / det;

    glm::vec3 cramer_t = ray.origin - v0;
    float u = glm::dot(cramer_t, cramer_p) * inv_det;
    if (u < 0 || u > 1) {
        // std::cout << "tri miss U" << std::endl;
        return false;
    }

    glm::vec3 cramer_q = glm::cross(cramer_t, AB);
    float v = glm::dot(ray.vector, cramer_q) * inv_det;
    if (v < 0 || (u + v) > 1) {
        // std::cout << "tri miss V" << std::endl;
        return false;
    }

    t = glm::dot(AC, cramer_q) * inv_det;

    // std::cout << "tri hit " << t << ' ' << u << ' ' << v << std::endl;
    return true;
}

glm::vec3 Triangle::normal(const glm::vec3& point) const {
    // TODO
    glm::vec3 a = v1 - v0;
    glm::vec3 b = v2 - v0;

    return glm::cross(a, b);
}