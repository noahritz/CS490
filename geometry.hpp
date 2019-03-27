#ifndef __GEOMETRY_HPP__
#define __GEOMETRY_HPP__

#include <glm/vec3.hpp>
#include "raytrace.hpp"

class Shape {
public:

    glm::vec3 color;

    Shape();
    Shape(glm::vec3 color);

    virtual bool intersect(const Ray& ray, float &t) const = 0;
    glm::vec3 surface(const glm::vec3& point, const Light& light) const;
    virtual glm::vec3 normal(const glm::vec3& point) const = 0;

};

class Sphere : public Shape {
public:

    glm::vec3 center;
    float radius;
    // glm::vec3 color;

    Sphere(glm::vec3 ctr, float r);
    Sphere(glm::vec3 ctr, glm::vec3 col, float r);

    bool intersect(const Ray& ray, float &t) const;
    // glm::vec3 surface(const glm::vec3& point, const Light& light) const;
    glm::vec3 normal(const glm::vec3& point) const;

private:

    bool solveQuadratic (const float &a, const float &b, const float &c, float &x0, float &x1) const;

};

class Triangle : public Shape {
public:

    glm::vec3 v0;
    glm::vec3 v1;
    glm::vec3 v2;
    // glm::vec3 color;

    Triangle(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);
    Triangle(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 col);

    bool intersect(const Ray& ray, float &t) const;
    // glm::vec3 surface(const glm::vec3& point, const Light& light) const;
    glm::vec3 normal(const glm::vec3& point) const;

private:

};

#include "geometry.cpp"

#endif