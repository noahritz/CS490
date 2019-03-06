#ifndef __GEOMETRY_HPP__
#define __GEOMETRY_HPP__

#include "raytrace.hpp"
#include <glm/vec3.hpp>

class Shape {
public:

    virtual bool intersect(const Ray& ray, float &t) const = 0;
    virtual glm::vec3 surface(const glm::vec3& point, const Light& light) const = 0;
    virtual glm::vec3 normal(const glm::vec3& point) const = 0;

};

class Sphere : public Shape {
public:

    glm::vec3 center;
    float radius;
    glm::vec3 color;

    Sphere(glm::vec3 c, float r);

    bool intersect(const Ray& ray, float &t) const;
    glm::vec3 surface(const glm::vec3& point, const Light& light) const;
    glm::vec3 normal(const glm::vec3& point) const;

private:

    bool solveQuadratic (const float &a, const float &b, const float &c, float &x0, float &x1) const;

};

#include "geometry.cpp"

#endif