#include <memory>
#include <string>
#include <map>

#include <glm/glm.hpp>

namespace g3d {


struct Camera {

  virtual ~Camera() {};

  virtual glm::mat4 compute() = 0;

  virtual glm::vec3 lookAt() { return glm::vec3{0,0,0}; };

  virtual void keyInput() {};

  virtual void scrollInput(double x, double y) {};

  virtual void select(const std::string &preset) {};

};

std::shared_ptr<Camera> makeLookat(float scale,
                                   const glm::vec3 &initial_position,
                                   bool z_is_up = false);



std::shared_ptr<Camera> makeRotCamera(float scale,
                                      glm::vec3 lookat,
                                      const std::map<std::string, glm::mat4> &presets = {});
}
