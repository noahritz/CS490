#ifndef __GEOMETRY_HPP__
#define __GEOMETRY_HPP__

#include <vector>
#include <glm/vec3.hpp>
#include "raytrace.hpp"
#include "CImg.h"

class Shape {
public:

    glm::vec3 color;
    float lambert;
    float specular;

    bool model;
    Intersection intersected_tri[4];

    Shape();
    Shape(glm::vec3 color);
    Shape(glm::vec3 color, float lam, float spec);
    virtual ~Shape();

    virtual bool intersect(const Ray& ray, float &t) = 0;
    virtual glm::vec3 surface(const Ray& ray, const glm::vec3& point, const std::vector<Shape*>& objects, const std::vector<Light*> &lights, Grid &grid) const;
    virtual glm::vec3 normal(const glm::vec3& point, const Ray& ray) const = 0;
    virtual glm::vec3 min() const = 0;
    virtual glm::vec3 max() const = 0;


};

class Sphere : public Shape {
public:

    glm::vec3 center;
    float radius;
    // glm::vec3 color;

    Sphere(glm::vec3 ctr, float r);
    Sphere(glm::vec3 ctr, float r, glm::vec3 col);
    Sphere(glm::vec3 ctr, float r, glm::vec3 col, float lam, float spec);

    bool intersect(const Ray& ray, float &t);
    glm::vec3 normal(const glm::vec3& point, const Ray& ray) const;
    glm::vec3 min() const;
    glm::vec3 max() const;

private:

    bool solveQuadratic (const float &a, const float &b, const float &c, float &x0, float &x1) const;

};

class Triangle : public Shape {
public:

    glm::vec3 v0;
    glm::vec3 v1;
    glm::vec3 v2;

    Triangle(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);
    Triangle(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 col);
    Triangle(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 col, float lam, float spec);

    bool intersect(const Ray& ray, float &t);
    glm::vec3 normal(const glm::vec3& point, const Ray& ray) const;
    glm::vec3 min() const;
    glm::vec3 max() const;

};

class TexturedTriangle : public Triangle {
public:

    bool bottom;
    glm::vec3 UV[4];
    cimg_library::CImg<float>& texture;

    TexturedTriangle(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float lam, float spec, cimg_library::CImg<float>& tex, bool bot);

    bool intersect(const Ray& ray, float &t) override;
    glm::vec3 surface(const Ray& ray, const glm::vec3& point, const std::vector<Shape*>& objects, const std::vector<Light*> &lights, Grid &grid) const override;
};

class Model : public Shape {
public:

    glm::vec3 minimum;
    glm::vec3 maximum;
    std::vector<Triangle*> triangles;

    Model();
    bool intersect(const Ray& ray, float &t);
    glm::vec3 normal(const glm::vec3& point, const Ray& ray) const;
    glm::vec3 min() const;
    glm::vec3 max() const;

};

#include "geometry.cpp"

#endif