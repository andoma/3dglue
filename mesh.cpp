#include "mesh.hpp"

#include <memory>
#include <vector>
#include <stdexcept>

#include <string.h>
#include <glm/gtx/normal.hpp>

#include "mappedfile.hpp"

namespace g3d {

static void
backref_vertex(uint32_t point, uint32_t ti, Mesh &md)
{
    // Scans the inverse index to find an empty slot (set to magic value 3)
    // Then write tringle-index there
    // This shuold never fail (because we precalculate the ranges)

    const size_t offset = md.inverse_index_offset(point);
    const size_t size = md.inverse_index_count(point);
    for(size_t i = 0; i < size; i++) {
        if(md.m_inverse_index_tri[offset + i] == 3) {
            md.m_inverse_index_tri[offset + i] = ti;
            return;
        }
    }
    abort();
}

void
Mesh::inverse_index_update(bool compress)
{
    if(m_indicies.size() % 3)
        throw std::runtime_error{"Elements not a multiple of 3"};

    m_inverse_index.clear();
    m_inverse_index.resize(num_vertices(), 0);

    size_t used_vertices = 0;
    uint32_t max_use = 0;
    for(size_t i = 0; i < m_indicies.size(); i++) {
        const uint32_t use = ++m_inverse_index[m_indicies[i]];
        max_use = std::max(use, max_use);
        if(use == 1)
            used_vertices++;
        else if((int)use == INVERSE_INDEX_COUNT_MASK + 1)
            throw std::runtime_error{"Vertex shared by too many triangles"};
    }

    if(used_vertices == num_vertices() || !compress) {
        uint32_t offset = 0;
        for(size_t i = 0; i < m_inverse_index.size(); i++) {
            uint32_t size = m_inverse_index[i];
            if(offset >= (1U << (32 - INVERSE_INDEX_OFFSET_SHIFT)))
                throw std::runtime_error{"Vertex offset too large"};

            m_inverse_index[i] |= offset << INVERSE_INDEX_OFFSET_SHIFT;
            offset += size;
        }
        m_inverse_index_tri.clear();
        m_inverse_index_tri.resize(offset, 3);

        for(size_t i = 0; i < m_indicies.size(); i += 3) {
            size_t tri = i / 3;
            backref_vertex(m_indicies[i + 0], (tri << 2) + 0, *this);
            backref_vertex(m_indicies[i + 1], (tri << 2) + 1, *this);
            backref_vertex(m_indicies[i + 2], (tri << 2) + 2, *this);
        }

    } else {
        std::vector<float> new_attributes;
        new_attributes.resize(used_vertices * m_apv);

        std::vector<uint32_t> new_attribute_location;
        new_attribute_location.resize(m_attributes.size());

        size_t o = 0;
        uint32_t offset = 0;
        int removed = 0;
        for(size_t i = 0; i < m_inverse_index.size(); i++) {
            const uint32_t size = m_inverse_index[i];
            if(size == 0) {
                removed++;
                continue;
            }
            new_attribute_location[i] = o;
            memcpy(&new_attributes[o * m_apv], &m_attributes[i * m_apv],
                   sizeof(float) * m_apv);

            if(offset >= (1U << (32 - INVERSE_INDEX_OFFSET_SHIFT)))
                throw std::runtime_error{"Vertex offset too large"};

            m_inverse_index[o] = (offset << INVERSE_INDEX_OFFSET_SHIFT) | size;
            offset += size;
            o++;
        }

        m_inverse_index_tri.clear();
        m_inverse_index_tri.resize(offset, 3);

        for(size_t i = 0; i < m_indicies.size(); i += 3) {
            const size_t tri = i / 3;

            m_indicies[i + 0] = new_attribute_location[m_indicies[i + 0]];
            m_indicies[i + 1] = new_attribute_location[m_indicies[i + 1]];
            m_indicies[i + 2] = new_attribute_location[m_indicies[i + 2]];

            backref_vertex(m_indicies[i + 0], (tri << 2) + 0, *this);
            backref_vertex(m_indicies[i + 1], (tri << 2) + 1, *this);
            backref_vertex(m_indicies[i + 2], (tri << 2) + 2, *this);
        }
        m_attributes = std::move(new_attributes);
    }
}

void
Mesh::inverse_index_clear()
{
    m_inverse_index.clear();
    m_inverse_index_tri.clear();
}

void
Mesh::compute_normals()
{
    if(!has_normals())
        return;

    if(m_inverse_index_tri.size() == 0)
        inverse_index_update(true);

    const size_t num_triangles = m_indicies.size() / 3;

    std::vector<glm::vec3> normals;

    normals.reserve(num_triangles);

    for(size_t i = 0; i < m_indicies.size(); i += 3) {
        auto v0 = get_xyz(m_indicies[i + 0]);
        auto v1 = get_xyz(m_indicies[i + 1]);
        auto v2 = get_xyz(m_indicies[i + 2]);
        auto n = glm::cross(v0 - v1, v0 - v2);
        normals.push_back(n);
    }

    for(size_t i = 0; i < num_vertices(); i++) {
        const int offset = inverse_index_offset(i);
        const int size = inverse_index_count(i);

        glm::vec3 n{0, 0, 0};
        for(int j = 0; j < size; j++) {
            uint32_t ti = m_inverse_index_tri[offset + j] >> 2;
            n += normals[ti];
        }

        n = glm::normalize(n);

        m_attributes[i * m_apv + 3] = n.x;
        m_attributes[i * m_apv + 4] = n.y;
        m_attributes[i * m_apv + 5] = n.z;
    }
}

std::pair<glm::vec3, glm::vec3>
Mesh::aabb() const
{
    glm::vec3 min{INFINITY, INFINITY, INFINITY};
    glm::vec3 max{-INFINITY, -INFINITY, -INFINITY};

    for(size_t i = 0; i < num_vertices(); i++) {
        const auto v = get_xyz(i);
        min = glm::min(v, min);
        max = glm::max(v, max);
    }
    return std::make_pair(min, max);
}

void
Mesh::translate(const glm::vec3 &tvec)
{
    for(size_t i = 0; i < num_vertices(); i++) {
        set_xyz(i, get_xyz(i) + tvec);
    }
}

std::shared_ptr<Mesh>
Mesh::cube(const glm::vec3 &pos, float s, bool normals,
           const std::shared_ptr<Texture2D> &tex0)
{
    auto md = std::make_shared<Mesh>(normals ? MeshAttributes::Normals
                                             : MeshAttributes::None);

    md->m_attributes.resize(8 * md->m_apv);

    md->set_xyz(0, pos + glm::vec3{-s, -s, -s});
    md->set_xyz(1, pos + glm::vec3{s, -s, -s});
    md->set_xyz(2, pos + glm::vec3{s, s, -s});
    md->set_xyz(3, pos + glm::vec3{-s, s, -s});
    md->set_xyz(4, pos + glm::vec3{-s, -s, s});
    md->set_xyz(5, pos + glm::vec3{s, -s, s});
    md->set_xyz(6, pos + glm::vec3{s, s, s});
    md->set_xyz(7, pos + glm::vec3{-s, s, s});

    md->m_indicies.resize(12 * 3);

    // Front
    md->set_face(0, 0, 1, 5);
    md->set_face(1, 0, 5, 4);

    // Left
    md->set_face(2, 3, 0, 4);
    md->set_face(3, 3, 4, 7);

    // Right
    md->set_face(4, 1, 2, 6);
    md->set_face(5, 1, 6, 5);

    // Back
    md->set_face(6, 2, 3, 7);
    md->set_face(7, 2, 7, 6);

    // Top
    md->set_face(8, 4, 5, 6);
    md->set_face(9, 4, 6, 7);

    // Bottom
    md->set_face(10, 3, 2, 1);
    md->set_face(11, 3, 1, 0);

    if(normals)
        md->compute_normals();

    if(tex0) {
        md->set_uv0(0, glm::vec2{0, 1});
        md->set_uv0(1, glm::vec2{1, 1});
        md->set_uv0(4, glm::vec2{0, 0});
        md->set_uv0(5, glm::vec2{1, 0});

        md->set_uv0(2, glm::vec2{0, 1});
        md->set_uv0(3, glm::vec2{1, 1});
        md->set_uv0(6, glm::vec2{0, 0});
        md->set_uv0(7, glm::vec2{1, 0});
    }

    return md;
}

std::shared_ptr<Mesh>
Mesh::loadSTL(const char *path, const glm::mat4 transform)
{
    MappedFile f(path);
    if(f.size() < 84) {
        fprintf(stderr, "%s: short file\n", path);
        return nullptr;
    }

    const uint32_t num_triangles = *(uint32_t *)(f.data() + 80);
    if(f.size() != 84 + num_triangles * 50) {
        fprintf(stderr, "%s: incorrect length %zd expected %d\n", path,
                f.size(), 84 + num_triangles * 50);
        return nullptr;
    }

    auto m = std::make_shared<Mesh>(MeshAttributes::None);
    m->m_attributes.resize(num_triangles * m->m_apv * 3);

    const uint8_t *start = f.data() + 84;
    for(int i = 0; i < num_triangles; i++) {
        const float *f = (const float *)start;
        m->set_xyz(i * 3 + 0, transform * glm::vec4{f[1 * 3 + 0], f[1 * 3 + 1],
                                                    f[1 * 3 + 2], 1});
        m->set_xyz(i * 3 + 1, transform * glm::vec4{f[2 * 3 + 0], f[2 * 3 + 1],
                                                    f[2 * 3 + 2], 1});
        m->set_xyz(i * 3 + 2, transform * glm::vec4{f[3 * 3 + 0], f[3 * 3 + 1],
                                                    f[3 * 3 + 2], 1});
        start += 50;
    }

    return m;
}

}  // namespace g3d
