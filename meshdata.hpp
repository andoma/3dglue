#pragma once

#include <string>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

namespace g3d {

struct Texture2D;

struct MeshData {


  MeshData(bool normals, bool per_vertex_color,
           const std::shared_ptr<Texture2D> &tex0 = nullptr)
    : m_normals(normals)
    , m_per_vertex_color(per_vertex_color)
    , m_apv(3 +
            (normals ? 3 : 0) +
            (per_vertex_color ? 4 : 0) +
            (tex0 ? 2 : 0))
    , m_rgba_offset(3 + (normals ? 3 : 0))
    , m_uv0_offset(m_rgba_offset + (per_vertex_color ? 4 : 0))
    , m_tex0(tex0)
  {}

  MeshData(bool normals, bool per_vertex_color, bool uv)
    : m_normals(normals)
    , m_per_vertex_color(per_vertex_color)
    , m_apv(3 +
            (normals ? 3 : 0) +
            (per_vertex_color ? 4 : 0) +
            (uv ? 2 : 0))
    , m_rgba_offset(3 + (normals ? 3 : 0))
    , m_uv0_offset(m_rgba_offset + (per_vertex_color ? 4 : 0))
  {}

  const bool m_normals;
  const bool m_per_vertex_color;
  const int m_apv; // Attributes Per Vertex
  const size_t m_rgba_offset;
  const size_t m_uv0_offset;

  std::shared_ptr<Texture2D> m_tex0{nullptr};

  std::string m_name;

  glm::vec3 get_xyz(size_t vertex) const {

    return glm::vec3{m_attributes[vertex * m_apv + 0],
                     m_attributes[vertex * m_apv + 1],
                     m_attributes[vertex * m_apv + 2]};
  }

  void set_xyz(size_t vertex, const glm::vec3 &xyz) {

    m_attributes[vertex * m_apv + 0] = xyz.x;
    m_attributes[vertex * m_apv + 1] = xyz.y;
    m_attributes[vertex * m_apv + 2] = xyz.z;
  }

  void set_rgba(size_t vertex, const glm::vec4 &rgba) {

    m_attributes[vertex * m_apv + 0 + m_rgba_offset] = rgba.r;
    m_attributes[vertex * m_apv + 1 + m_rgba_offset] = rgba.g;
    m_attributes[vertex * m_apv + 2 + m_rgba_offset] = rgba.b;
    m_attributes[vertex * m_apv + 3 + m_rgba_offset] = rgba.a;
  }

  void set_uv0(size_t vertex, const glm::vec2 &uv) {

    m_attributes[vertex * m_apv + m_uv0_offset + 0] = uv.x;
    m_attributes[vertex * m_apv + m_uv0_offset + 1] = uv.y;
  }

  void set_face(size_t face,
                uint32_t v0, uint32_t v1, uint32_t v2) {

    m_indicies[face * 3 + 0] = v0;
    m_indicies[face * 3 + 1] = v1;
    m_indicies[face * 3 + 2] = v2;
  }


  size_t num_vertices() const {
    return m_attributes.size() / m_apv;
  }

  size_t num_triangles() const {
    return m_indicies.size() / 3;
  }

  std::vector<float> m_attributes;

  std::vector<uint32_t> m_indicies;


  // Reverse mapping point -> triangle

  void reverse_index(bool compress = false);

  const int VERTEX_INFO_SHIFT = 6; // Max 64 triangles can share a vertex

  const int VERTEX_INFO_MASK = (1 << VERTEX_INFO_SHIFT) - 1;

  std::vector<uint32_t> m_vertex_info;
  std::vector<uint32_t> m_vertex_to_tri;

  const size_t vertex_info_offset(int v) {
    return m_vertex_info[v] >> VERTEX_INFO_SHIFT;
  }

  const size_t vertex_info_size(int v) {
    return m_vertex_info[v] & VERTEX_INFO_MASK;
  }

  void compute_normals();

  void compute_normals(uint32_t max_distance);

  void group_triangles();

  void clear_reverse();

  void remove_triangles(const glm::vec3 &direction);

  void colorize_from_curvature();

  void find_neighbour_vertices(uint32_t start_vertex,
                               uint32_t max_distance,
                               std::vector<std::pair<uint32_t, uint32_t>> &output);

  void find_neighbour_triangles(uint32_t start_triangle,
                                uint32_t max_distance,
                                std::vector<std::pair<uint32_t, uint32_t>> &output);

  void find_neighbour_triangles_from_vertex(uint32_t start_vertex,
                                            uint32_t max_distance,
                                            std::vector<std::pair<uint32_t, uint32_t>> &output);


  static std::shared_ptr<MeshData> cube(const std::shared_ptr<Texture2D> &tex0 = nullptr);

};

}
