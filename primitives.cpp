#include "primitives.hpp"

namespace g3d {

std::pair<std::shared_ptr<VertexBuffer>, std::vector<glm::ivec3>>
icosphere(float r, int order)
{
    // set up a 20-triangle icosahedron
    const float f = r * (1 + sqrtf(5)) / 2.0f;

    std::vector<glm::vec3> vertices = {{-r, f, 0},  {r, f, 0},   {-r, -f, 0},
                                       {r, -f, 0},  {0, -r, f},  {0, r, f},
                                       {0, -r, -f}, {0, r, -f},  {f, 0, -r},
                                       {f, 0, r},   {-f, 0, -r}, {-f, 0, r}};

    std::vector<glm::ivec3> triangles = {
        {0, 11, 5},  {0, 5, 1},  {0, 1, 7},  {0, 7, 10}, {0, 10, 11},
        {11, 10, 2}, {5, 11, 4}, {1, 5, 9},  {7, 1, 8},  {10, 7, 6},
        {3, 9, 4},   {3, 4, 2},  {3, 2, 6},  {3, 6, 8},  {3, 8, 9},
        {9, 8, 1},   {4, 9, 5},  {2, 4, 11}, {6, 2, 10}, {8, 6, 7}};

    return {VertexBuffer::make(vertices), triangles};
}
};  // namespace g3d
