#include "camera.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glm/gtc/matrix_transform.hpp>


namespace g3d {



struct Lookat : public Camera {

  Lookat(float scale)
    : m_scale(scale)
    , m_cameraPos{0, scale * 0.5, scale}
  {}

  glm::mat4 compute()
  {
    ImGui::SliderFloat("X", &m_cameraPos.x, -m_scale, m_scale);
    ImGui::SliderFloat("Y", &m_cameraPos.y, -m_scale, m_scale);
    ImGui::SliderFloat("Z", &m_cameraPos.z, -m_scale, m_scale);

    glm::mat4 view = glm::lookAt(m_cameraPos,
                                 m_lookat,
                                 {0.0f,1.0f,0.0f});

    return view;
  }

  const float m_scale{100};

  float m_fov{45};

  glm::vec3 m_cameraPos;
  glm::vec3 m_lookat{0};

};


std::shared_ptr<Camera> makeLookat(float scale)
{
  return std::make_shared<Lookat>(scale);
}

}
