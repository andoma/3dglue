#if 0
#pragma once

#include <string>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

class thread_pool;

namespace g3d {

struct Texture2D;

enum class VertexAttribute {
    Position,
    Normal,
    Color,
    UV0,
};

struct VertexBuffer {
    virtual const float *get_attribute(VertexAttribute va, size_t index) const;
    virtual size_t get_stride(VertexAttribute va, size_t index) const;
};

#if 0



enum class MeshAttributes {
    None = 0x0,
    // Vertex usage is implicit
    Normals = 0x1,
    PerVertexColor = 0x2,
    UV0 = 0x04,
};

inline constexpr MeshAttributes
operator|(MeshAttributes a, MeshAttributes b)
{
    return static_cast<MeshAttributes>(static_cast<int>(a) |
                                       static_cast<int>(b));
}

inline constexpr MeshAttributes
operator&(MeshAttributes a, MeshAttributes b)
{
    return static_cast<MeshAttributes>(static_cast<int>(a) &
                                       static_cast<int>(b));
}

inline constexpr bool
operator!(MeshAttributes a)
{
    return static_cast<bool>(!static_cast<int>(a));
}

inline constexpr bool
has_normals(MeshAttributes a)
{
    return !!(a & MeshAttributes::Normals);
}

inline constexpr bool
has_per_vertex_color(MeshAttributes a)
{
    return !!(a & MeshAttributes::PerVertexColor);
}

inline constexpr bool
has_uv0(MeshAttributes a)
{
    return !!(a & MeshAttributes::UV0);
}

struct Mesh {
    Mesh(MeshAttributes flags, const std::shared_ptr<Texture2D> &tex0 = nullptr)
      : m_attr_flags(flags |
                     (tex0 ? MeshAttributes::UV0 : MeshAttributes::None))
      , m_apv(3 + (has_normals() ? 3 : 0) + (has_per_vertex_color() ? 4 : 0) +
              (has_uv0() ? 2 : 0))
      , m_rgba_offset(3 + (has_normals() ? 3 : 0))
      , m_uv0_offset(m_rgba_offset + (has_per_vertex_color() ? 4 : 0))
    {
    }

    bool has_normals() const { return g3d::has_normals(m_attr_flags); }

    bool has_per_vertex_color() const
    {
        return g3d::has_per_vertex_color(m_attr_flags);
    }

    bool has_uv0() const { return g3d::has_uv0(m_attr_flags); }

    const MeshAttributes m_attr_flags;
    const int m_apv;  // Attributes Per Vertex

    const size_t m_normal_offset = 3;
    const size_t m_rgba_offset;
    const size_t m_uv0_offset;

    std::shared_ptr<Texture2D> m_tex0{nullptr};

    std::string m_name;

    glm::vec3 get_xyz(size_t vertex) const
    {
        return glm::vec3{m_attributes[vertex * m_apv + 0],
                         m_attributes[vertex * m_apv + 1],
                         m_attributes[vertex * m_apv + 2]};
    }

    void set_xyz(size_t vertex, const glm::vec3 &xyz)
    {
        m_attributes[vertex * m_apv + 0] = xyz.x;
        m_attributes[vertex * m_apv + 1] = xyz.y;
        m_attributes[vertex * m_apv + 2] = xyz.z;
    }

    void set_normal(size_t vertex, const glm::vec3 &xyz)
    {
        m_attributes[vertex * m_apv + m_normal_offset + 0] = xyz.x;
        m_attributes[vertex * m_apv + m_normal_offset + 1] = xyz.y;
        m_attributes[vertex * m_apv + m_normal_offset + 2] = xyz.z;
    }

    void set_rgba(size_t vertex, const glm::vec4 &rgba)
    {
        m_attributes[vertex * m_apv + 0 + m_rgba_offset] = rgba.r;
        m_attributes[vertex * m_apv + 1 + m_rgba_offset] = rgba.g;
        m_attributes[vertex * m_apv + 2 + m_rgba_offset] = rgba.b;
        m_attributes[vertex * m_apv + 3 + m_rgba_offset] = rgba.a;
    }

    void set_uv0(size_t vertex, const glm::vec2 &uv)
    {
        m_attributes[vertex * m_apv + m_uv0_offset + 0] = uv.x;
        m_attributes[vertex * m_apv + m_uv0_offset + 1] = uv.y;
    }

    void set_face(size_t face, uint32_t v0, uint32_t v1, uint32_t v2)
    {
        m_indicies[face * 3 + 0] = v0;
        m_indicies[face * 3 + 1] = v1;
        m_indicies[face * 3 + 2] = v2;
    }

    // Not very efficient interface, but sometimes useful
    size_t push_triangle(const glm::vec3 &v0, const glm::vec3 &v1,
                         const glm::vec3 &v2,
                         const glm::vec4 &rgba = glm::vec4{1})
    {
        size_t vi = num_vertices();
        m_attributes.resize(m_attributes.size() + m_apv * 3);

        size_t ii = m_indicies.size();
        m_indicies.resize(m_indicies.size() + 3);

        for(const auto &v : (const glm::vec3[]){v0, v1, v2}) {
            set_xyz(vi, v);
            if(has_normals())
                set_normal(vi, glm::vec3{0});
            if(has_per_vertex_color())
                set_rgba(vi, rgba);
            if(has_uv0())
                set_uv0(vi, glm::vec2{0});
            m_indicies[ii++] = vi;
            vi++;
        }
        return vi - 3;
    }

    size_t num_vertices() const { return m_attributes.size() / m_apv; }

    size_t num_triangles() const { return m_indicies.size() / 3; }

    std::vector<float> m_attributes;

    std::vector<uint32_t> m_indicies;

    std::pair<glm::vec3, glm::vec3> aabb() const;

    void translate(const glm::vec3 &tvec);

    void transform(const glm::mat4 &mtx);

    void compute_normals();

    // Inverse mapping point -> triangle

    void inverse_index_update(bool compress = false);

    void inverse_index_clear();

    const int INVERSE_INDEX_OFFSET_SHIFT =
        6;  // Max 64 triangles can share a vertex

    const int INVERSE_INDEX_COUNT_MASK = (1 << INVERSE_INDEX_OFFSET_SHIFT) - 1;

    /**
     * This word is split on bit INVERSE_INDEX_OFFSET_SHIFT
     *
     *  Currently this means that a vertex can only reference
     *  (1 << INVERSE_INDEX_OFFSET_SHIFT) number of triangles.
     *
     *
     * + ------------+-------+
     * | Offset      | Count |
     * +-------------+-------+
     *
     */
    std::vector<uint32_t> m_inverse_index;

    /**
     * This word is split with upper 30 bits pointing to triangle
     * and bottom two the triangle's vertex index (0,1,2)
     * The value 3 is reserved and used during index construction
     *
     * + ------------+-------------+
     * | Triangle    | 2bit Vertex |
     * +-------------+-------------+
     *
     */
    std::vector<uint32_t> m_inverse_index_tri;

    const size_t inverse_index_offset(int v)
    {
        return m_inverse_index[v] >> INVERSE_INDEX_OFFSET_SHIFT;
    }

    const size_t inverse_index_count(int v)
    {
        return m_inverse_index[v] & INVERSE_INDEX_COUNT_MASK;
    }

    static std::shared_ptr<Mesh> cuboid(
        const glm::vec3 &pos, const glm::vec3 &size, bool normals = false,
        const std::shared_ptr<Texture2D> &tex0 = nullptr);

    static std::shared_ptr<Mesh> cube(
        const glm::vec3 &pos, float size, bool normals = false,
        const std::shared_ptr<Texture2D> &tex0 = nullptr)
    {
        return cuboid(pos, glm::vec3{size}, normals, tex0);
    }

    static std::shared_ptr<Mesh> loadOBJ(const char *path,
                                         glm::mat4 transform = glm::mat4{1});

    static std::shared_ptr<Mesh> loadSTL(const char *path,
                                         glm::mat4 transform = glm::mat4{1});
};
#endif

}  // namespace g3d
#endif
