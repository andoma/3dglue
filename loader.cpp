#include <utility>
#include <memory>
#include <stdio.h>
#include <string.h>
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

std::shared_ptr<VertexBuffer>
loadPCD(const char *path, const glm::mat4 transform,
        glm::vec3 bbmin, glm::vec3 bbmax)
{
    FILE *fp = fopen(path, "r");
    if(fp == NULL)
        throw std::system_error(errno, std::system_category());

    std::vector<glm::vec3> vertices;

    char line[1024];

    int height = -1;
    int width = -1;
    int points = -1;

    while(1) {
        if(fgets(line, sizeof(line), fp) == NULL)
            throw std::runtime_error{"Premature end of file"};
        if(line[0] == '#')
            continue;
        bool got_lf = false;
        for(size_t i = 0; i < sizeof(line); i++) {
            if(line[i] == '\n') {
                line[i] = 0;
                got_lf = true;
            }
        }
        if(!got_lf) {
            throw std::runtime_error{"Too long line"};
        }

        sscanf(line, "HEIGHT %u", &height);
        sscanf(line, "WIDTH %u", &width);
        sscanf(line, "POINTS %u", &points);

        if(!strcmp(line, "DATA binary"))
            break;
    }
    if(points < 1)
        throw std::runtime_error{"Unknown/Bad number of points"};

    vertices.resize(points);
    size_t c = fread(vertices.data(), sizeof(glm::vec3), points, fp);
    if(c != points) {
        throw std::runtime_error{"Short read"};
    }

    fclose(fp);

    size_t j = 0;
    for(size_t i = 0; i < vertices.size(); i++) {
        auto p = transform * glm::vec4{vertices[i], 1};
        if(p.x < bbmin.x ||
           p.y < bbmin.y ||
           p.z < bbmin.z ||
           p.x > bbmax.x ||
           p.y > bbmax.y ||
           p.z > bbmax.z)
            continue;
        vertices[j++] = p;
    }

    vertices.resize(j);
    return VertexBuffer::make(vertices);
}

}  // namespace g3d
