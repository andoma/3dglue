#include <iostream>

#include "camera.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glm/gtc/matrix_transform.hpp>


namespace g3d {



struct Lookat : public Camera {

  Lookat(float scale,
         const glm::vec3 &initial_position,
         bool z_is_up)
    : m_scale(scale)
    , m_cameraPos{initial_position}
    , m_up{z_is_up ? glm::vec3{0.0f,0.0f,1.0f} : glm::vec3{0.0f,1.0f,0.0f}}
  {}

  glm::vec3 lookAt() override
  {
    return m_lookat;
  };

  glm::mat4 compute() override
  {
    ImGui::Text("Camera Position");
    ImGui::PushID("cpos");
    ImGui::SliderFloat("X", &m_cameraPos.x, -m_scale, m_scale);
    ImGui::SliderFloat("Y", &m_cameraPos.y, -m_scale, m_scale);
    ImGui::SliderFloat("Z", &m_cameraPos.z, -m_scale, m_scale);
    ImGui::PopID();

    ImGui::Text("Look at");
    ImGui::PushID("clook");
    ImGui::SliderFloat("X", &m_lookat.x, -m_scale, m_scale);
    ImGui::SliderFloat("Y", &m_lookat.y, -m_scale, m_scale);
    ImGui::SliderFloat("Z", &m_lookat.z, -m_scale, m_scale);
    ImGui::PopID();

    glm::mat4 view = glm::lookAt(m_cameraPos,
                                 m_lookat,
                                 m_up);

    return view;
  }

  const float m_scale{100};

  glm::vec3 m_cameraPos;
  glm::vec3 m_lookat{0};
  glm::vec3 m_up;

};


std::shared_ptr<Camera> makeLookat(float scale,
                                   const glm::vec3 &initial_position,
                                   bool z_is_up)
{
  return std::make_shared<Lookat>(scale, initial_position,  z_is_up);
}






struct RotCamera : public Camera {

  RotCamera(float scale, glm::vec3 lookat,
            const std::vector<CameraPreset> &presets)
    : m_scale(scale)
    , m_distance(-scale / 2)
    , m_height(0)
    , m_lookat(lookat)
    , m_presets(presets)
  {}

  glm::vec3 lookAt() override
  {
    return m_lookat;
  };

  glm::mat4 compute() override
  {
    ImGui::Text("Camera ");
    ImGui::SliderAngle("Azimuth", &m_azimuth);
    ImGui::SliderFloat("Height", &m_height, -m_scale, m_scale);
    ImGui::SliderFloat("Distance", &m_distance, -m_scale, m_scale);


    ImGui::Text("Look at");
    ImGui::SliderFloat("X", &m_lookat.x, -m_scale, m_scale);
    ImGui::SliderFloat("Y", &m_lookat.y, -m_scale, m_scale);
    ImGui::SliderFloat("Z", &m_lookat.z, -m_scale, m_scale);


    if(m_presets.size()) {

      if(ImGui::Button("Goto")) {
        ImGui::OpenPopup("Goto");
      }

      if(ImGui::BeginPopup("Goto")) {
        for(const auto &p : m_presets) {
          if(ImGui::Button(p.name.c_str())) {
            ImGui::CloseCurrentPopup();

            auto r = p.pos - p.lookat;
            m_lookat = p.lookat;

            m_height = r.z;
            m_azimuth = atan2f(-r.x, -r.y);
            m_distance = -sqrtf(r.x * r.x + r.y * r.y);
            break;
          }
        }
        ImGui::EndPopup();
      }
    }

    glm::vec3 relativePos = glm::vec3(m_distance * sin(m_azimuth),
                                      m_distance * cos(m_azimuth),
                                      m_height);

    glm::mat4 view = glm::lookAt(relativePos + m_lookat,
                                 m_lookat,
                                 glm::vec3{0,0,1});

    return view;
  }

  void scrollInput(double x, double y) override {
    m_distance += y * m_scale * 0.05;
    m_azimuth += x * 0.25;

    if(m_azimuth <= -M_PI * 2)
      m_azimuth += M_PI * 2;

    if(m_azimuth >= M_PI * 2)
      m_azimuth -= M_PI * 2;
  }

  const float m_scale;
  float m_distance;
  float m_height;

  float m_azimuth{0};

  glm::vec3 m_lookat;

  const std::vector<CameraPreset> m_presets;
};


std::shared_ptr<Camera> makeRotCamera(float scale, glm::vec3 lookat,
                                      const std::vector<CameraPreset> &presets)
{
  return std::make_shared<RotCamera>(scale, lookat, presets);
}


}
