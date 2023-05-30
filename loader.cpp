#include <utility>
#include <memory>
#include <stdio.h>
#include <system_error>

#include "vertexbuffer.hpp"

namespace g3d {

std::pair<std::shared_ptr<VertexBuffer>,
          std::shared_ptr<std::vector<glm::ivec3>>>
loadOBJ(const char *path, const glm::mat4 transform)
{
    FILE *fp = fopen(path, "r");
    if(fp == NULL)
        throw std::system_error(errno, std::system_category());

    std::vector<glm::vec3> vertices;
    auto triangles = std::make_shared<std::vector<glm::ivec3>>();

    while(!feof(fp)) {
        double a, b, c;
        char x;
        if(fscanf(fp, "%c %lf %lf %lf\n", &x, &a, &b, &c) == 4) {
            if(x == 'v') {
                auto v = transform * glm::vec4{a, b, c, 1};
                vertices.push_back(glm::vec3{v});
            } else if(x == 'f') {
                triangles->push_back(glm::ivec3{a - 1, b - 1, c - 1});
            }
        }
    }
    fclose(fp);
    return {VertexBuffer::make(vertices), triangles};
}

}  // namespace g3d
