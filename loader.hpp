#ifndef __LOADER_HPP__
#define __LOADER_HPP__

#include <vector>
#include <string>
#include <glm/vec3.hpp>
#include "geometry.hpp"

void load(std::vector<Shape *>& objects, std::string filename, glm::vec3 color, float lambert, float specular);

#include "loader.cpp"

#endif