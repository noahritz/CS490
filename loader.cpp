#include <iostream>
#include <fstream>
#include <sstream>
#include "loader.hpp"

using std::ifstream;
using std::string;
using std::stringstream;

void load(std::vector<Shape *>& objects, std::string filename, float scale, glm::vec3 location, glm::vec3 color, float lambert, float specular) {
    string line;
    string type;
    string v1, v2, v3;
    int ind1, ind2, ind3;

    Model *model = new Model();
    glm::vec3 min = {10000.0, 10000.0, 10000.0};
    glm::vec3 max = {-10000.0, -10000.0, -10000.0};

    ifstream f;
    f.open(filename);
    std::vector<glm::vec3> vertices;
    while (getline(f, line)) {
        stringstream linestream{line};
        linestream >> type;
        if (type == "v") {
            glm::vec3 v;
            linestream >> v.x >> v.y >> v.z;
            v *= scale;
            v += location;
            min = glm::min(v, min);
            max = glm::max(v, max);
            vertices.push_back(v);
        } else if (type == "f") {
            linestream >> v1 >> v2 >> v3;
            sscanf(v1.c_str(), "%i//", &ind1);
            sscanf(v2.c_str(), "%i//", &ind2);
            sscanf(v3.c_str(), "%i//", &ind3);
            ind1--;
            ind2--;
            ind3--;
            Triangle *tri = new Triangle{vertices[ind1], vertices[ind2], vertices[ind3], color, lambert, specular};
            model->triangles.push_back(tri);
            // objects.push_back(tri);
        } else {
            continue;
        }
    }

    model->minimum = min;
    model->maximum = max;
    objects.push_back(model);
}