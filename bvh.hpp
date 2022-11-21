#pragma once

#include <optional>

namespace g3d {

enum class IntersectionMode { POINT };

struct Intersector {
    virtual ~Intersector(){};

    virtual std::optional<std::pair<size_t, glm::vec3>> intersect(
        const glm::vec3 &origin, const glm::vec3 &direction) const = 0;

    static std::shared_ptr<Intersector> make(
        const std::shared_ptr<VertexBuffer> &vb, IntersectionMode mode);
};

}  // namespace g3d
