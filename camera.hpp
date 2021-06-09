#include <memory>

#include <glm/glm.hpp>

namespace g3d {

struct Camera {

  virtual ~Camera() {};

  virtual glm::mat4 compute() = 0;

  virtual void keyInput() {};

};

std::shared_ptr<Camera> makeLookat(float scale);

}
