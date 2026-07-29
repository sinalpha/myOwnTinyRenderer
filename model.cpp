#include <fstream>
#include <sstream>
#include "model.h"

Model::Model(const std::string filename) {
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
            vec3 v;
            for (int i : {0,1,2}) iss >> v[i];
            verts.push_back(v);
        }
        else if (!line.compare(0, 3, "vt ")) {
            iss >> trash >> trash;
            vec2 v;
            for (int i : {0, 1}) iss >> v[i];
            UVs.push_back({v.x, 1-v.y}); // the obj v axis points up, the image y axis points down
        
        }
        else if (!line.compare(0, 3, "vn ")) {
            iss >> trash >> trash;
            vec3 v;
            for (int i : {0, 1, 2}) iss >> v[i];
            normals.push_back(normalized(v));
        }
        else if (!line.compare(0, 2, "f ")) {
            int f,t,n, cnt = 0;
            iss >> trash;
            while (iss >> f >> trash >> t >> trash >> n) {
                facet_vrt.push_back(--f);
                facet_uv.push_back(--t);
                facet_nrml.push_back(--n);
                cnt++;
            }
            if (3!=cnt) {
                std::cerr << "Error: the obj file is supposed to be triangulated" << std::endl;
                return;
            }
        }
    }
    std::cerr << "# v# " << nverts() << " f# "  << nfaces() << std::endl;
}

int Model::nverts() const { return verts.size(); }
int Model::nfaces() const { return facet_vrt.size()/3; }

vec3 Model::vert(const int i) const {
    return verts[i];
}

vec3 Model::vert(const int iface, const int nthvert) const {
    return verts[facet_vrt[iface*3+nthvert]];
}

