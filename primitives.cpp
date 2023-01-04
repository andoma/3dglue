#include "primitives.hpp"
#include <map>

namespace g3d {

std::pair<std::shared_ptr<VertexBuffer>, std::vector<glm::ivec3>>
icosphere(float radius, int order)
{
    // set up a 20-triangle icosahedron
    const float r = 1.0f;
    const float f = (1 + sqrtf(5)) / 2.0f;
    const float T = powf(4, order);

    std::vector<glm::vec3> vertices = {{-r, f, 0},  {r, f, 0},   {-r, -f, 0},
                                       {r, -f, 0},  {0, -r, f},  {0, r, f},
                                       {0, -r, -f}, {0, r, -f},  {f, 0, -r},
                                       {f, 0, r},   {-f, 0, -r}, {-f, 0, r}};

    std::vector<glm::ivec3> triangles = {
        {0, 11, 5},  {0, 5, 1},  {0, 1, 7},  {0, 7, 10}, {0, 10, 11},
        {11, 10, 2}, {5, 11, 4}, {1, 5, 9},  {7, 1, 8},  {10, 7, 6},
        {3, 9, 4},   {3, 4, 2},  {3, 2, 6},  {3, 6, 8},  {3, 8, 9},
        {9, 8, 1},   {4, 9, 5},  {2, 4, 11}, {6, 2, 10}, {8, 6, 7}};

    std::map<uint32_t, uint32_t> vertex_cache;

    auto add_pt = [&](uint32_t a, uint32_t b) {
        const uint32_t key = ((a + b) * (a + b + 1) / 2) + glm::min(a, b);
        auto it = vertex_cache.find(key);
        if(it != vertex_cache.end()) {
            return it->second;
        }
        uint32_t n = vertices.size();
        vertices.push_back((vertices[a] + vertices[b]) * 0.5f);
        vertex_cache[key] = n;
        return n;
    };
    for(int i = 0; i < order; i++) {
        std::vector<glm::ivec3> nt;
        for(size_t k = 0; k < triangles.size(); k++) {
            const auto t = triangles[k];
            const uint32_t a = add_pt(t.x, t.y);
            const uint32_t b = add_pt(t.y, t.z);
            const uint32_t c = add_pt(t.z, t.x);
            nt.push_back({t.x, a, c});
            nt.push_back({t.y, b, a});
            nt.push_back({t.z, c, b});
            nt.push_back({a, b, c});
        }
        triangles = nt;
    }

    for(size_t i = 0; i < vertices.size(); i++) {
        vertices[i] = glm::normalize(vertices[i]) * radius;
    }

    return {VertexBuffer::make(vertices), triangles};
}
};  // namespace g3d
