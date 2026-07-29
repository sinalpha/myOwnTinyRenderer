#include <vector>
#include "geometry.h"

class Model {
    std::vector<vec3> verts = {};    // array of vertices
    std::vector<int> facet_vrt = {}; // per-triangle index in the above array
    std::vector<vec3> normals = {};
    std::vector<int> facet_nrml = {};
    std::vector<vec2> UVs = {};
    std::vector<int> facet_uv = {};
    
public:
    Model(const std::string filename);
    int nverts() const; // number of vertices
    int nfaces() const; // number of triangles
    vec3 vert(const int i) const;                          // 0 <= i < nverts()
    vec3 vert(const int iface, const int nthvert) const;   // 0 <= iface <= nfaces(), 0 <= nthvert < 3
    vec3 nrml(const int iface, const int nthnrml) const {
        return normals[facet_nrml[iface * 3 + nthnrml]];
    }
    vec2 UV(const int iface, const int nthUV) const {
        return UVs[facet_uv[iface * 2 + nthUV]];
    }
};

