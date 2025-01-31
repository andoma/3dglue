#include "arraybuffer.hpp"

namespace g3d {

void
VertexAttribBuffer::load(const VertexBuffer &src)
{
    const float *data = src.get_attributes(VertexAttribute::Position);
    size_t elements_per_vertex = src.get_elements(VertexAttribute::Position);

    bool packed = true;
    for(size_t i = 1; i < 32; i++) {
        const float *n = src.get_attributes((VertexAttribute)i);
        if(n == NULL)
            continue;  // Not in use

        if(n != data + elements_per_vertex) {
            packed = false;
        }

        elements_per_vertex += src.get_elements((VertexAttribute)i);
    }

    std::vector<float> copybuf;

    const size_t vertices = src.size();
    m_byte_stride = elements_per_vertex * sizeof(float);

    if(!packed) {
        copybuf.resize(elements_per_vertex * vertices);

        float *dst = copybuf.data();
        size_t offset = 0;

        for(size_t i = 0; i < 32; i++) {
            const float *s = src.get_attributes((VertexAttribute)i);
            if(s == NULL) {
                m_ose.push_back(std::make_tuple(0, 0));
                continue;  // Not in use
            }
            const size_t elements = src.get_elements((VertexAttribute)i);
            const size_t src_stride = src.get_stride((VertexAttribute)i);
            assert(src_stride != 0);

            for(size_t j = 0; j < vertices; j++) {
                for(size_t k = 0; k < elements; k++) {
                    dst[j * elements_per_vertex + k] = s[j * src_stride + k];
                }
            }

            m_ose.push_back(std::make_tuple(offset, elements));

            dst += elements;
            offset += elements;
        }
        data = copybuf.data();
    } else {
        size_t offset = 0;

        m_ose.clear();

        for(size_t i = 0; i < 32; i++) {
            const float *s = src.get_attributes((VertexAttribute)i);
            if(s == NULL) {
                m_ose.push_back(std::make_tuple(0, 0));
                continue;
            }

            const size_t elements = src.get_elements((VertexAttribute)i);
            assert(src.get_stride((VertexAttribute)i) == elements_per_vertex);

            m_ose.push_back(std::make_tuple(offset, elements));
            offset += elements;
        }
    }

    m_buf.write((const float *)data,
                vertices * elements_per_vertex * sizeof(float));
    m_count = vertices;
}

bool
VertexAttribBuffer::bind()
{
    return m_buf.bind();
}

void
VertexAttribBuffer::ptr(GLuint index, VertexAttribute va) const
{
    glVertexAttribPointer(index, get_elements(va), GL_FLOAT, GL_TRUE,
                          m_byte_stride, get_attributes(va));
}

size_t
VertexAttribBuffer::size() const
{
    return m_count;
}

const float *
VertexAttribBuffer::get_attributes(VertexAttribute va) const
{
    const size_t index = (size_t)va;
    if(index >= m_ose.size() || std::get<1>(m_ose[index]) == 0)
        return nullptr;
    return (const float *)(intptr_t)(std::get<0>(m_ose[index]) * sizeof(float));
}

size_t
VertexAttribBuffer::get_stride(VertexAttribute va) const
{
    return m_byte_stride / sizeof(float);
}

size_t
VertexAttribBuffer::get_elements(VertexAttribute va) const
{
    const size_t index = (size_t)va;
    if(index >= m_ose.size() || std::get<1>(m_ose[index]) == 0)
        return 0;
    return std::get<1>(m_ose[index]);
}

}  // namespace g3d
