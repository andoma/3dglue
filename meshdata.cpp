#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <string.h>

#include "meshdata.hpp"

#include <glm/gtx/normal.hpp>

namespace g3d {

static void
backref_vertex(uint32_t point, uint32_t ti, MeshData &md)
{
  const size_t offset = md.vertex_info_offset(point);
  const size_t size = md.vertex_info_size(point);
  for(size_t i = 0; i < size; i++) {
    if(md.m_vertex_to_tri[offset + i] == 3) {
      md.m_vertex_to_tri[offset + i] = ti;
      return;
    }
  }
  abort();
}


void MeshData::reverse_index(bool compress)
{
  if(m_indicies.size() % 3)
    throw std::runtime_error{"Elements not a multiple of 3"};

  m_vertex_info.clear();
  m_vertex_info.resize(num_vertices(), 0);

  size_t used_vertices = 0;
  uint32_t max_use = 0;
  for(size_t i = 0; i < m_indicies.size(); i++) {
    // We only support a single vertex sharing 255 triangles
    // Could be fixed by using 64bit instead

    const uint32_t use = ++m_vertex_info[m_indicies[i]];
    max_use = std::max(use, max_use);
    if(use == 1)
      used_vertices++;
    else if((int)use == VERTEX_INFO_MASK + 1)
      throw std::runtime_error{"Vertex shared by too many triangles"};
  }

  if(used_vertices == num_vertices() || !compress) {

    uint32_t offset = 0;
    for(size_t i = 0; i < m_vertex_info.size(); i++) {
      uint32_t size = m_vertex_info[i];
      if(offset >= (1U << (32 - VERTEX_INFO_SHIFT)))
        throw std::runtime_error{"Vertex offset too large"};

      m_vertex_info[i] |= offset << VERTEX_INFO_SHIFT;
      offset += size;
    }
    m_vertex_to_tri.clear();
    m_vertex_to_tri.resize(offset, 3);

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
    for(size_t i = 0; i < m_vertex_info.size(); i++) {
      const uint32_t size = m_vertex_info[i];
      if(size == 0) {
        removed++;
        continue;
      }
      new_attribute_location[i] = o;
      memcpy(&new_attributes[o * m_apv],
             &m_attributes[i * m_apv],
             sizeof(float) * m_apv);


      if(offset >= (1U << (32 - VERTEX_INFO_SHIFT)))
        throw std::runtime_error{"Vertex offset too large"};

      m_vertex_info[o] = (offset << VERTEX_INFO_SHIFT) | size;
      offset += size;
      o++;
    }

    m_vertex_to_tri.clear();
    m_vertex_to_tri.resize(offset, 3);

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


void MeshData::remove_triangles(const glm::vec3 &direction)
{
  size_t o = 0;
  for(size_t i = 0; i < m_indicies.size(); i+= 3) {
    int p0 = m_indicies[i + 0];
    int p1 = m_indicies[i + 1];
    int p2 = m_indicies[i + 2];

    auto n = glm::triangleNormal(get_xyz(p0),
                                 get_xyz(p1),
                                 get_xyz(p2));
    float a = glm::dot(n, direction);
    if(a < 0.5f) {
      m_indicies[o++] = p0;
      m_indicies[o++] = p1;
      m_indicies[o++] = p2;
    }
  }
  m_indicies.resize(o);
  clear_reverse();
}


void MeshData::compute_normals()
{
  if(!m_normals)
    return;

  if(m_vertex_to_tri.size() == 0)
    reverse_index(true);

  const size_t num_triangles = m_indicies.size() / 3;

  std::vector<glm::vec3> normals;

  normals.reserve(num_triangles);

  for(size_t i = 0; i < m_indicies.size(); i+= 3) {
    auto v0 = get_xyz(m_indicies[i + 0]);
    auto v1 = get_xyz(m_indicies[i + 1]);
    auto v2 = get_xyz(m_indicies[i + 2]);
    auto n = glm::cross(v0 - v1, v0 - v2);
    normals.push_back(n);
  }

  for(size_t i = 0; i < num_vertices(); i++) {
    const int offset = vertex_info_offset(i);
    const int size = vertex_info_size(i);

    glm::vec3 n{0,0,0};
    for(int j = 0; j < size; j++) {
      uint32_t ti = m_vertex_to_tri[offset + j] >> 2;
      n += normals[ti];
    }

    n = glm::normalize(n);

    m_attributes[i * m_apv + 3] = n.x;
    m_attributes[i * m_apv + 4] = n.y;
    m_attributes[i * m_apv + 5] = n.z;
  }
}


void MeshData::colorize_from_curvature()
{
  if(!m_per_vertex_color)
    return;

  int color_offset = m_normals ? 6 : 3;

  if(m_vertex_to_tri.size() == 0)
    reverse_index(true);

  const size_t num_triangles = m_indicies.size() / 3;

  std::vector<glm::vec3> normals;

  normals.reserve(num_triangles);

  for(size_t i = 0; i < m_indicies.size(); i+= 3) {
    auto v0 = get_xyz(m_indicies[i + 0]);
    auto v1 = get_xyz(m_indicies[i + 1]);
    auto v2 = get_xyz(m_indicies[i + 2]);
   //  auto n = glm::cross(v0 - v1, v0 - v2);
    auto n = glm::triangleNormal(v0, v1, v2);
    normals.push_back(n);
  }

  for(size_t i = 0; i < num_vertices(); i++) {
    const int offset = vertex_info_offset(i);
    const int size = vertex_info_size(i);

    uint32_t ti = m_vertex_to_tri[offset + 0] >> 2;
    auto n0 = normals[ti];

    float s = 0;
    for(int j = 1; j < size; j++) {
      uint32_t ti = m_vertex_to_tri[offset + j] >> 2;
      auto n = normals[ti];
      s += 1.0f - glm::dot(n, n0);
    }
#if 0
    if(s > 0.5) {
      for(int j = 0; j < size; j++) {
        uint32_t ti = m_vertex_to_tri[offset + j] >> 2;
        m_indicies[3 * ti + 0] = 0;
        m_indicies[3 * ti + 1] = 0;
        m_indicies[3 * ti + 2] = 0;
      }
    }
#endif
    m_attributes[i * m_apv + color_offset + 0] = s;
    m_attributes[i * m_apv + color_offset + 1] = 1;
    m_attributes[i * m_apv + color_offset + 2] = 0;

  }
}




static int
scan_triangle(MeshData &md, uint32_t ti, std::vector<int> &ttg, int group)
{
  int count = 1;
  std::vector<int> todo;
  todo.push_back(ti);
  ttg[ti] = group;

  while(!todo.empty()) {
    uint32_t ti = todo[todo.size() - 1];
    todo.pop_back();

    std::unordered_map<int, int> neighbours;
    for(int p = 0; p < 3; p++) {
      uint32_t vertex = md.m_indicies[ti * 3 + p];
      const size_t offset = md.vertex_info_offset(vertex);
      const size_t size = md.vertex_info_size(vertex);

      for(size_t i = 0; i < size; i++) {
        uint32_t n = md.m_vertex_to_tri[offset + i] >> 2;
        if(n == ti || ttg[n] != 0)
          continue;
        if(++neighbours[n] == 2) {
          ttg[n] = group;
          count++;
          todo.push_back(n);
        }
      }
    }
  }
  return count;
}

void MeshData::group_triangles()
{
  if(m_vertex_to_tri.size() == 0)
    reverse_index(true);

  const int num_triangles = m_indicies.size() / 3;

  std::vector<int> ttg(num_triangles, 0);

  int biggest_group_number = 0;
  int biggest_group_count = 0;

  int next_group = 1;
  for(int ti = 0; ti < num_triangles; ti++) {
    if(ttg[ti] == 0) {
      int count = scan_triangle(*this, ti, ttg, next_group);
      if(count > biggest_group_count) {
        biggest_group_count = count;
        biggest_group_number = next_group;
      }
      next_group++;
    }
  }

  std::vector<uint32_t> new_mesh;

  for(int ti = 0; ti < num_triangles; ti++) {
    if(ttg[ti] == biggest_group_number) {
      new_mesh.push_back(m_indicies[ti * 3 + 0]);
      new_mesh.push_back(m_indicies[ti * 3 + 1]);
      new_mesh.push_back(m_indicies[ti * 3 + 2]);
    }
  }

  m_indicies = std::move(new_mesh);
  clear_reverse();
}


void MeshData::clear_reverse()
{
  m_vertex_info.clear();
  m_vertex_to_tri.clear();
}




void MeshData::compute_normals2()
{
  if(!m_normals)
    return;

  if(m_vertex_to_tri.size() == 0)
    reverse_index(true);

  const size_t num_triangles = m_indicies.size() / 3;

  std::vector<glm::vec3> normals;

  normals.reserve(num_triangles);

  for(size_t i = 0; i < m_indicies.size(); i+= 3) {
    auto v0 = get_xyz(m_indicies[i + 0]);
    auto v1 = get_xyz(m_indicies[i + 1]);
    auto v2 = get_xyz(m_indicies[i + 2]);
    auto n = glm::cross(v0 - v1, v0 - v2);
    normals.push_back(n);
  }

  for(size_t i = 0; i < num_vertices(); i++) {
    const int offset = vertex_info_offset(i);
    const int size = vertex_info_size(i);

    std::unordered_set<uint32_t> local_mesh;

    for(int j = 0; j < size; j++) {
      uint32_t ti = m_vertex_to_tri[offset + j] >> 2;
      if(local_mesh.find(ti) != local_mesh.end())
        continue;
      local_mesh.insert(ti);

      for(int k = 0; k < 3; k++) {
        uint32_t i2 = m_indicies[3 * ti + k];
        if(i2 == i)
          continue;

        const int offset = vertex_info_offset(i2);
        const int size = vertex_info_size(i2);

        for(int l = 0; l < size; l++) {
          uint32_t ti = m_vertex_to_tri[offset + l] >> 2;
          local_mesh.insert(ti);
          if(local_mesh.find(ti) != local_mesh.end())
            continue;

          for(int m = 0; m < 3; m++) {
            uint32_t i3 = m_indicies[3 * ti + m];
            if(i3 == i2 || i3 == i)
              continue;

            const int offset = vertex_info_offset(i3);
            const int size = vertex_info_size(i3);

            for(int n = 0; n < size; n++) {
              uint32_t ti = m_vertex_to_tri[offset + n] >> 2;
              local_mesh.insert(ti);
            }
          }
        }
      }
    }


    glm::vec3 n{0,0,0};

    for(auto ti : local_mesh) {
      n += normals[ti];
    }
    n = glm::normalize(n);

    m_attributes[i * m_apv + 3] = n.x;
    m_attributes[i * m_apv + 4] = n.y;
    m_attributes[i * m_apv + 5] = n.z;
  }
}



}
