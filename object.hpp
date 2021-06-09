#include <memory>

#include <glm/glm.hpp>

namespace g3d {

struct Object {

  virtual ~Object() {};

  virtual void draw(const glm::mat4&) = 0;

  virtual void setModelMatrix(const glm::mat4&) = 0;

  virtual void setColor(const glm::vec4&) {};

};


std::shared_ptr<Object> makeCheckerBoard(float tilesize,
                                         int num_tiles_per_dimention);

}
