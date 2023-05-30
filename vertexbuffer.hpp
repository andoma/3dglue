#pragma once

#include <string>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

namespace g3d {

enum class VertexAttribute {
    Position,
    Normal,
    Color,
    UV0,
    Aux,
};

struct VertexBuffer {
    virtual ~VertexBuffer(){};
    virtual size_t size() const = 0;
    virtual const float *get_attributes(VertexAttribute va) const = 0;
    virtual size_t get_stride(VertexAttribute va) const = 0;
    virtual size_t get_elements(VertexAttribute va) const = 0;

    uint32_t get_attribute_mask() const;

    static std::shared_ptr<VertexBuffer> make(
        const std::vector<glm::vec3> &pos);

    static std::shared_ptr<VertexBuffer> make(
        const std::vector<glm::vec3> &positions,
        const std::vector<glm::vec4> &colors);
};

}  // namespace g3d
