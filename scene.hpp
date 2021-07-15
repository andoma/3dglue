#pragma once

#include <memory>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

namespace g3d {

struct Object;
struct Camera;

struct Scene {

  virtual ~Scene() {};

  virtual bool prepare() = 0;

  virtual void draw() = 0;

  std::shared_ptr<Camera> m_camera;

  std::vector<std::shared_ptr<Object>> m_objects;

  glm::mat4 m_P{1};

  glm::mat4 m_V{1};
};


std::shared_ptr<Scene> makeGLFWImguiScene(const char *title,
                                          int width, int height);

}
