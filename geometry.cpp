#include <glm/geometric.hpp>
#include <glm/exponential.hpp>
#include <vector>
#include <algorithm>
#include "CImg.h"
#include "geometry.hpp"

using cimg_library::CImg;

/* SHAPE */

Shape::Shape() : color(glm::vec3{1.0, 1.0, 1.0}), model(false) {}
Shape::Shape(glm::vec3 col) : color(col), lambert(1.0), specular(0.0), model(false) {}
Shape::Shape(glm::vec3 col, float lam, float spec) : color(col), lambert(lam), specular(spec), model(false) {}
Shape::~Shape() {}

glm::vec3 Shape::surface(const Ray& ray, const glm::vec3& point, const std::vector<Shape*>& objects, const std::vector<Light*> &lights, Grid &grid) const {
    glm::vec3 _color, lambert_color, specular_color;
    lambert_color = specular_color = glm::vec3{0.0, 0.0, 0.0};

    glm::vec3 norm = this->normal(point);

    if (lambert) {
        for (auto &l : lights) {

            if (l->visible(point + (norm * 0.01f), objects, grid, norm)) {
                float contribution = glm::dot(glm::normalize(l->position - point), norm);
                if (contribution > 0) {
                    lambert_color += (l->color * contribution);
                }
            }
        }

        lambert_color *= this->color; // scale it by the object's color
    }

    if (specular) {
        glm::vec3 reflected_vec = glm::reflect(ray.vector, norm);
        Ray reflected_ray{point + (reflected_vec * 0.01f), reflected_vec};
        reflected_ray.depth = ray.depth + 1;

        specular_color = trace(reflected_ray, objects, lights, grid);
    }

    _color = (lambert_color * lambert) + (specular_color * specular);

    return _color;
}


/* SPHERE */

// Constructor for a white sphere
Sphere::Sphere(glm::vec3 ctr, float r) : center{ctr}, radius{r} {}

// Constructor for a sphere of specified color
Sphere::Sphere(glm::vec3 ctr, float r, glm::vec3 col) : Shape(col), center{ctr}, radius{r} {}

// Constructor for a sphere of specified color and material
Sphere::Sphere(glm::vec3 ctr, float r, glm::vec3 col, float lam, float spec) : Shape(col, lam, spec), center{ctr}, radius{r} {}

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
bool Sphere::intersect(const Ray &ray, float &t) {
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

glm::vec3 Sphere::normal(const glm::vec3& point) const {
    return glm::normalize(point - center);
}

glm::vec3 Sphere::min() const {
    return glm::vec3{center.x - radius, center.y - radius, center.z - radius};
}

glm::vec3 Sphere::max() const {
    return glm::vec3{center.x + radius, center.y + radius, center.z + radius};
}

/* TRIANGLE */

Triangle::Triangle(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2) :
    v0(p0), v1(p1), v2(p2) {}
Triangle::Triangle(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 col) :
    Shape(col), v0(p0), v1(p1), v2(p2) {}
Triangle::Triangle(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 col, float lam, float spec) :
    Shape(col, lam, spec), v0(p0), v1(p1), v2(p2) {}

// From https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
bool Triangle::intersect(const Ray& ray, float &t) {

    glm::vec3 AB = v1 - v0;
    glm::vec3 AC = v2 - v0;
    glm::vec3 cramer_p = glm::cross(ray.vector, AC);
    float det = glm::dot(AB, cramer_p);
    
    // Disregard triangle if triangle is backfacing
    if (det < 0.0000001) {
        return false;
    }

    float inv_det = 1.0 / det;

    glm::vec3 cramer_t = ray.origin - v0;
    float u = glm::dot(cramer_t, cramer_p) * inv_det;
    if (u < 0 || u > 1) {
        return false;
    }

    glm::vec3 cramer_q = glm::cross(cramer_t, AB);
    float v = glm::dot(ray.vector, cramer_q) * inv_det;
    if (v < 0 || (u + v) > 1) {
        return false;
    }

    t = glm::dot(AC, cramer_q) * inv_det;

    return true;
}

glm::vec3 Triangle::normal(const glm::vec3& point) const {
    glm::vec3 a = v1 - v0;
    glm::vec3 b = v2 - v0;

    return glm::normalize(glm::cross(a, b));
}

glm::vec3 Triangle::min() const {
    return glm::vec3{std::min({v0.x, v1.x, v2.x}), std::min({v0.y, v1.y, v2.y}), std::min({v0.z, v1.z, v2.z})};
}

glm::vec3 Triangle::max() const {
    return glm::vec3{std::max({v0.x, v1.x, v2.x}), std::max({v0.y, v1.y, v2.y}), std::max({v0.z, v1.z, v2.z})};
}

/* MODEL */
Model::Model() {model = true;};

bool Model::intersect(const Ray& ray, float &t) {
    float ti, tj; // Dummy variables for the box intersection
    if (!ray.intersectBox(minimum, maximum, ti, tj)) {
        return false;
    }

    float t_model = 10000.0;
    float t_test;

    int thread = omp_get_thread_num();
    intersected_tri[thread].hit = false;
    
    for (auto &tri : triangles) {
        if (tri->intersect(ray, t_test) && t_test < t_model && t_test >= 0) {
            t_model = t_test;
            intersected_tri[thread].hit = true;
            intersected_tri[thread].obj = tri;
            intersected_tri[thread].point = ray.origin + (ray.vector * t_model);
            intersected_tri[thread].t = t_model;
        }
    }

    if (intersected_tri[thread].hit) {
        t = t_model;
        return true;
    }

    return false;
}

glm::vec3 Model::normal(const glm::vec3 &point) const {
    return intersected_tri[omp_get_thread_num()].obj->normal(point);
}

glm::vec3 Model::min() const {
    return minimum;
}

glm::vec3 Model::max() const {
    return maximum;
}

/* TEXTURED RECTANGLE */
TexturedTriangle::TexturedTriangle(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, float lam, float spec, CImg<float>& tex, bool bot) :
    Triangle(p0, p1, p2, glm::vec3{1.0, 1.0, 1.0}, lam, spec), texture(tex), bottom(bot) {
}

bool TexturedTriangle::intersect(const Ray& ray, float &t) {

    glm::vec3 AB = v1 - v0;
    glm::vec3 AC = v2 - v0;
    glm::vec3 cramer_p = glm::cross(ray.vector, AC);
    float det = glm::dot(AB, cramer_p);
    
    // Disregard triangle if triangle is backfacing
    if (det < 0.0000001) {
        return false;
    }

    float inv_det = 1.0 / det;

    glm::vec3 cramer_t = ray.origin - v0;
    float u = glm::dot(cramer_t, cramer_p) * inv_det;
    if (u < 0 || u > 1) {
        return false;
    }

    glm::vec3 cramer_q = glm::cross(cramer_t, AB);
    float v = glm::dot(ray.vector, cramer_q) * inv_det;
    if (v < 0 || (u + v) > 1) {
        return false;
    }

    t = glm::dot(AC, cramer_q) * inv_det;

    if (bottom) {
        UV[omp_get_thread_num()] = (u * glm::vec3{0.0, 0.0, 0.0}) + (v * glm::vec3{0.0, 1.0, 0.0}) + ((1.0f - u - v) * glm::vec3{1.0, 1.0, 0.0});
    } else {
        UV[omp_get_thread_num()] = (u * glm::vec3{1.0, 0.0, 0.0}) + (v * glm::vec3{0.0, 0.0, 0.0}) + ((1.0f - u - v) * glm::vec3{1.0, 1.0, 0.0});
    }

    return true;
}

glm::vec3 TexturedTriangle::surface(const Ray& ray, const glm::vec3& point, const std::vector<Shape*>& objects, const std::vector<Light*> &lights, Grid &grid) const {
    glm::vec3 _color, lambert_color, specular_color;
    lambert_color = specular_color = glm::vec3{0.0, 0.0, 0.0};

    glm::vec3 norm = this->normal(point);

    if (lambert) {
        for (auto &l : lights) {

            if (l->visible(point + (norm * 0.01f), objects, grid, norm)) {
                float contribution = glm::dot(glm::normalize(l->position - point), norm);
                if (contribution > 0) {
                    lambert_color += (l->color * contribution);
                }
            }
        }

        // Get texture color
        glm::ivec3 tex_coord{UV[omp_get_thread_num()] * glm::vec3{texture.width(), texture.height(), 0.0}};
        glm::vec3 tex_color{texture(tex_coord.x, tex_coord.y, 0, 0), texture(tex_coord.x, tex_coord.y, 0, 1), texture(tex_coord.x, tex_coord.y, 0, 2)};
        lambert_color *= tex_color/255.0f;

    }

    if (specular) {
        glm::vec3 reflected_vec = glm::reflect(ray.vector, norm);
        Ray reflected_ray{point + (reflected_vec * 0.01f), reflected_vec};
        reflected_ray.depth = ray.depth + 1;

        specular_color = trace(reflected_ray, objects, lights, grid);
    }

    _color = (lambert_color * lambert) + (specular_color * specular);

    return _color;
}