#pragma once

#include <memory>
#include <vector>

#include <glm/glm.hpp>

namespace g3d {

struct MeshData {


  MeshData(bool normals, bool per_vertex_color)
    : m_normals(normals)
    , m_per_vertex_color(per_vertex_color)
    , m_apv(3 +
            (normals ? 3 : 0) +
            (per_vertex_color ? 4 : 0))
  {}

  const bool m_normals;
  const bool m_per_vertex_color;
  const int m_apv; // Attributes Per Vertex

  glm::vec3 get_xyz(size_t vertex) const {

    return glm::vec3{m_attributes[vertex * m_apv + 0],
                     m_attributes[vertex * m_apv + 1],
                     m_attributes[vertex * m_apv + 2]};
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

  void compute_normals2();

  void group_triangles();

  void clear_reverse();

  void remove_triangles(const glm::vec3 &direction);

  void colorize_from_curvature();

};

}
