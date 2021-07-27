#pragma once

#include <memory>
#include <vector>

#include "object.hpp"

namespace g3d {


struct MeshData {

  std::vector<float> m_attributes;

  std::vector<uint32_t> m_indicies;

  bool m_normals{false};
  bool m_per_vertex_color{false};

  // Optional for (reverse) mapping point -> triangle

  std::vector<uint32_t> m_vertex_info;
  std::vector<uint32_t> m_vertex_to_tri;

};


std::shared_ptr<Object> makeMesh(std::unique_ptr<MeshData> &data);


}
