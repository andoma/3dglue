#pragma once

#include <GL/glew.h>

#include "vertexbuffer.hpp"

namespace g3d {

struct ArrayBuffer {
    ArrayBuffer(const ArrayBuffer &) = delete;
    ArrayBuffer &operator=(const ArrayBuffer &) = delete;

    ArrayBuffer(GLenum target) : m_target(target) {}

    ~ArrayBuffer()
    {
        if(m_buffer)
            glDeleteBuffers(1, &m_buffer);
    }

    bool bind()
    {
        if(!m_buffer)
            return false;
        glBindBuffer(m_target, m_buffer);
        return true;
    }

    void write(const void *ptr, size_t len)
    {
        if(!m_buffer)
            glGenBuffers(1, &m_buffer);

        glBindBuffer(m_target, m_buffer);
        glBufferData(m_target, len, ptr, GL_STATIC_DRAW);
    }

private:
    const GLenum m_target;
    GLuint m_buffer{0};
};

struct VertexAttribBuffer : public VertexBuffer {
    void load(const VertexBuffer &vb);

    bool bind();

    void ptr(GLuint index, VertexAttribute va) const;

    size_t size() const override;

    const float *get_attributes(VertexAttribute va) const override;

    size_t get_stride(VertexAttribute va) const override;

    size_t get_elements(VertexAttribute va) const override;

private:
    ArrayBuffer m_buf{GL_ARRAY_BUFFER};
    size_t m_count{0};
    size_t m_byte_stride{0};
    std::vector<std::tuple<size_t, size_t>> m_ose;  // <offset, elements>
};

}  // namespace g3d
