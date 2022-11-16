#include "vertexbuffer.hpp"

#include <glm/gtc/type_ptr.hpp>

namespace g3d {

uint32_t
VertexBuffer::get_attribute_mask() const
{
    uint32_t mask = 0;
    for(int i = 0; i < 32; i++) {
        if(get_elements((VertexAttribute)i)) {
            mask |= (1 << i);
        }
    }
    return mask;
}

struct VertexBufferCpu : public VertexBuffer {
    size_t size() const override { return m_positions.size(); }
    const float *get_attributes(VertexAttribute va) const override
    {
        switch(va) {
        case VertexAttribute::Position:
            return (const float *)m_positions.data();
        default:
            return nullptr;
        }
    }

    virtual size_t get_elements(VertexAttribute va) const override
    {
        switch(va) {
        case VertexAttribute::Position:
            return 3;
        default:
            return 0;
        }
    }

    virtual size_t get_stride(VertexAttribute va) const override
    {
        return get_elements(va);
    }

    std::vector<glm::vec3> m_positions;
};

std::shared_ptr<VertexBuffer>
VertexBuffer::make(const std::vector<glm::vec3> &positions)
{
    auto vbc = std::make_shared<VertexBufferCpu>();
    vbc->m_positions = positions;
    return vbc;
}

}  // namespace g3d
