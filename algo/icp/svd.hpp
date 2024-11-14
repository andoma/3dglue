#pragma once

#include <glm/glm.hpp>

namespace g3d {

struct SVD3 {
    SVD3(const glm::mat3& m);

    glm::mat3 U;
    glm::vec3 S;
    glm::mat3 V;
};

}
