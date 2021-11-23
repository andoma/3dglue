#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <string.h>

#include "meshdata.hpp"
#include "ext/thread-pool/thread_pool.hpp"

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


void MeshData::compute_normals(uint32_t max_distance, thread_pool &tp)
{
  if(!m_normals)
    return;

  if(m_vertex_to_tri.size() == 0)
    reverse_index(true);

  if(num_vertices() < 1)
    return;

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

  tp.parallelize_loop((size_t)0, num_vertices(), [&](const size_t& start, const size_t& end) {
    std::vector<std::pair<uint32_t, uint32_t>> triangles;
    for(size_t i = start; i < end; i++) {

      find_neighbour_triangles_from_vertex(i, max_distance, triangles);

      glm::vec3 n{0,0,0};

      for(auto ti : triangles) {
        n += normals[ti.second];
      }
      n = glm::normalize(n);
      m_attributes[i * m_apv + 3] = n.x;
      m_attributes[i * m_apv + 4] = n.y;
      m_attributes[i * m_apv + 5] = n.z;
    }
  });
}


void MeshData::find_neighbour_vertices(uint32_t start_vertex,
                                       uint32_t max_distance,
                                       std::vector<std::pair<uint32_t, uint32_t>> &output)
{
  std::unordered_set<uint32_t> visited;

  if(m_vertex_to_tri.size() == 0)
    reverse_index(true);

  visited.insert(start_vertex);
  output.clear();
  output.push_back(std::make_pair(0, start_vertex));

  for(size_t i = 0; i < output.size(); i++) {
    const uint32_t distance = output[i].first;
    if(distance == max_distance)
      continue;

    const uint32_t vertex = output[i].second;
    const size_t offset = vertex_info_offset(vertex);
    const size_t size = vertex_info_size(vertex);

    for(size_t i = 0; i < size; i++) {
      const uint32_t ti = m_vertex_to_tri[offset + i] >> 2;
      const uint32_t tix = m_vertex_to_tri[offset + i] & 0x3;

      for(uint32_t j = tix + 1; j < tix + 3; j++) {
        uint32_t next = m_indicies[3 * ti + (j % 3)];
        if(visited.find(next) != visited.end())
          continue;
        visited.insert(next);
        output.push_back(std::make_pair(distance + 1, next));
      }
    }
  }
}

void MeshData::find_neighbour_triangles(uint32_t start_triangle,
                                        uint32_t max_distance,
                                        std::vector<std::pair<uint32_t, uint32_t>> &output)
{
  std::unordered_set<uint32_t> visited;

  if(m_vertex_to_tri.size() == 0)
    reverse_index(true);

  visited.insert(start_triangle);
  output.clear();
  output.push_back(std::make_pair(0, start_triangle));

  for(size_t i = 0; i < output.size(); i++) {
    const uint32_t distance = output[i].first;
    if(distance == max_distance)
      continue;

    const uint32_t triangle = output[i].second;

    for(size_t j = 0; j < 3; j++) {
      const uint32_t vertex = m_indicies[3 * triangle + j];
      const size_t offset = vertex_info_offset(vertex);
      const size_t size = vertex_info_size(vertex);

      for(size_t i = 0; i < size; i++) {
        const uint32_t next = m_vertex_to_tri[offset + i] >> 2;
        if(visited.find(next) != visited.end())
          continue;
        visited.insert(next);
        output.push_back(std::make_pair(distance + 1, next));
      }
    }
  }
}

void MeshData::find_neighbour_triangles_from_vertex(uint32_t start_vertex,
                                                    uint32_t max_distance,
                                                    std::vector<std::pair<uint32_t, uint32_t>> &output)
{
  std::unordered_set<uint32_t> visited;

  if(m_vertex_to_tri.size() == 0)
    reverse_index(true);


  const size_t offset = vertex_info_offset(start_vertex);
  const size_t size = vertex_info_size(start_vertex);

  output.clear();

  for(size_t i = 0; i < size; i++) {
    const uint32_t ti = m_vertex_to_tri[offset + i] >> 2;
    visited.insert(ti);
    output.push_back(std::make_pair(0, ti));
  }

  for(size_t i = 0; i < output.size(); i++) {
    const uint32_t distance = output[i].first;
    if(distance == max_distance)
      continue;

    const uint32_t triangle = output[i].second;

    for(size_t j = 0; j < 3; j++) {
      const uint32_t vertex = m_indicies[3 * triangle + j];
      const size_t offset = vertex_info_offset(vertex);
      const size_t size = vertex_info_size(vertex);

      for(size_t i = 0; i < size; i++) {
        const uint32_t next = m_vertex_to_tri[offset + i] >> 2;
        if(visited.find(next) != visited.end())
          continue;
        visited.insert(next);
        output.push_back(std::make_pair(distance + 1, next));
      }
    }
  }
}

std::shared_ptr<MeshData> MeshData::cube(const std::shared_ptr<Texture2D> &tex0)
{
  auto md = std::make_shared<MeshData>(true, false, tex0);

  md->m_attributes.resize(8 * md->m_apv);

  float s = 0.5f;
  md->set_xyz(0, glm::vec3{-s, -s, -s});
  md->set_xyz(1, glm::vec3{ s, -s, -s});
  md->set_xyz(2, glm::vec3{ s,  s, -s});
  md->set_xyz(3, glm::vec3{-s,  s, -s});
  md->set_xyz(4, glm::vec3{-s, -s,  s});
  md->set_xyz(5, glm::vec3{ s, -s,  s});
  md->set_xyz(6, glm::vec3{ s,  s,  s});
  md->set_xyz(7, glm::vec3{-s,  s,  s});

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

  md->compute_normals();

  if(tex0) {

    md->set_uv0(0, glm::vec2{0,1});
    md->set_uv0(1, glm::vec2{1,1});
    md->set_uv0(4, glm::vec2{0,0});
    md->set_uv0(5, glm::vec2{1,0});

    md->set_uv0(2, glm::vec2{0,1});
    md->set_uv0(3, glm::vec2{1,1});
    md->set_uv0(6, glm::vec2{0,0});
    md->set_uv0(7, glm::vec2{1,0});
  }

  return md;
}

}
