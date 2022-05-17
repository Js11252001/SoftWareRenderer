#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"

Model::Model(const char* filename) : verts_(), faces_() {
    std::ifstream in;
    in.open(filename, std::ifstream::in);
    if (in.fail()) return;
    std::string line;
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;
        if (!line.compare(0, 2, "v ")) {
            iss >> trash;
            Vec3f v;
            for (int i = 0; i < 3; i++) iss >> v[i];
            verts_.push_back(v);
        }
        else if (!line.compare(0, 2, "f ")) {
            std::vector<int> f;
            std::vector<int> v;
            int itrash, idx, idx1;
            iss >> trash;
            while (iss >> idx >> trash >> idx1 >> trash >> itrash) {
                idx--; // in wavefront obj all indices start at 1, not zero
                idx1--;
                f.push_back(idx);
                v.push_back(idx1);
            }
            faces_.push_back(make_pair(f, v));
        }
        else if (!line.compare(0, 3, "vt ")) {
            iss >> trash >> trash;
            Vec3f v;
            for (int i = 0; i < 3; i++) iss >> v[i];
            tex_.push_back(v);
        }
    }
    std::cerr << "# v# " << verts_.size() << " f# " << faces_.size() << std::endl;
}

Model::~Model() {
}

int Model::nverts() {
    return (int)verts_.size();
}

int Model::nfaces() {
    return (int)faces_.size();
}

std::pair<std::vector<int>, std::vector<int>> Model::face(int idx) {
    return faces_[idx];
}

Vec3f Model::tex(int i) {
    return tex_[i];
}

Vec3f Model::vert(int i) {
    return verts_[i];
}
