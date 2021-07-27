#pragma once

#include <memory>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace g3d {

struct Object {

  virtual ~Object() {};

  virtual void prepare() {}

  virtual void draw(const glm::mat4 &P, const glm::mat4 &V) = 0;

  virtual void setColor(const glm::vec4&) {}

  void setModelMatrix(const glm::mat4 &m)
  {
    m_model_matrix = m;
  }

  void setModelMatrixScale(float scale)
  {
    m_model_matrix = glm::scale(glm::mat4{1}, {scale, scale, scale});
  }

  glm::mat4 m_model_matrix{1};

  std::string m_name;
};

std::shared_ptr<Object> makeCheckerBoard(float tilesize,
                                         int num_tiles_per_dimention);

std::shared_ptr<Object> makePointCloud(size_t num_points,
                                       const float *xyz,
                                       const float *rgb = NULL);

std::shared_ptr<Object> makeCube();

std::shared_ptr<Object> makeCross();

}
