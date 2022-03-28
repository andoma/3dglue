#include <iostream>

#include "camera.hpp"
#include "opengl.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace g3d {

struct Lookat : public Camera {
    Lookat(float scale, const glm::vec3 &initial_position, bool z_is_up)
      : m_scale(scale)
      , m_cameraPos{initial_position}
      , m_up{z_is_up ? glm::vec3{0.0f, 0.0f, 1.0f}
                     : glm::vec3{0.0f, 1.0f, 0.0f}}
    {
    }

    glm::vec3 lookAt() override { return m_lookat; };

    glm::vec3 camPosition() override { return m_cameraPos; }

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

        glm::mat4 view = glm::lookAt(m_cameraPos, m_lookat, m_up);

        return view;
    }

    const float m_scale{100};

    glm::vec3 m_cameraPos;
    glm::vec3 m_lookat{0};
    glm::vec3 m_up;
};

std::shared_ptr<Camera>
makeLookat(float scale, const glm::vec3 &initial_position, bool z_is_up)
{
    return std::make_shared<Lookat>(scale, initial_position, z_is_up);
}

struct RotCamera : public Camera {
    RotCamera(float scale, glm::vec3 lookat,
              const std::map<std::string, glm::mat4> &presets)
      : m_scale(scale)
      , m_distance(-scale)
      , m_height(0)
      , m_lookat(lookat)
      , m_presets(presets)
    {
    }

    glm::vec3 lookAt() override { return m_lookat; }

    glm::vec3 camPosition() override { return m_campos; }

    glm::mat4 compute() override
    {
        ImGui::Text("Camera ");
        ImGui::SliderAngle("Azimuth", &m_azimuth);
        ImGui::SliderFloat("Height", &m_height, -m_scale, m_scale);
        ImGui::SliderFloat("Distance", &m_distance, -m_scale * 2, 0);

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
                    if(ImGui::Button(p.first.c_str())) {
                        ImGui::CloseCurrentPopup();
                        select(p.second);
                        break;
                    }
                }
                ImGui::EndPopup();
            }
        }

        glm::vec3 relativePos = glm::vec3(
            m_distance * sin(m_azimuth), m_distance * cos(m_azimuth), m_height);

        m_campos = relativePos + m_lookat;

        glm::mat4 view = glm::lookAt(m_campos, m_lookat, glm::vec3{0, 0, 1});

        return view;
    }

    void scrollInput(double x, double y) override
    {
        m_distance += y * m_scale * 0.05;
        m_azimuth += x * 0.25;

        if(m_azimuth <= -M_PI * 2)
            m_azimuth += M_PI * 2;

        if(m_azimuth >= M_PI * 2)
            m_azimuth -= M_PI * 2;
    }

    void select(const glm::mat4 &m)
    {
        auto pos = m[3];
        auto dir = glm::normalize(m[1]);  // Positive Y

        auto lookat = pos + dir * 1000.0f;

        auto r = pos - lookat;
        m_lookat = lookat;
        m_height = r.z;
        m_azimuth = atan2f(-r.x, -r.y);
        m_distance = -sqrtf(r.x * r.x + r.y * r.y);
    }

    void select(const std::string &preset) override
    {
        auto it = m_presets.find(preset);
        if(it != m_presets.end())
            select(it->second);
    }

    void positionStore(int slot) override
    {
        char name[PATH_MAX];
        snprintf(name, sizeof(name), ".rotcam%d", slot);
        FILE *fp = fopen(name, "w");
        fprintf(fp, "%f %f %f %f %f %f\n", m_distance, m_height, m_azimuth,
                m_lookat.x, m_lookat.y, m_lookat.z);
        fclose(fp);
    }

    void positionRecall(int slot) override
    {
        char name[PATH_MAX];
        snprintf(name, sizeof(name), ".rotcam%d", slot);
        FILE *fp = fopen(name, "r");
        if(fp) {
            if(fscanf(fp, "%f %f %f %f %f %f\n", &m_distance, &m_height,
                      &m_azimuth, &m_lookat.x, &m_lookat.y, &m_lookat.z)) {
            }
            fclose(fp);
        }
    }

    const float m_scale;
    float m_distance;
    float m_height;

    float m_azimuth{0};

    glm::vec3 m_campos;
    glm::vec3 m_lookat;

    const std::map<std::string, glm::mat4> m_presets;
};

std::shared_ptr<Camera>
makeRotCamera(float scale, glm::vec3 lookat,
              const std::map<std::string, glm::mat4> &presets)
{
    return std::make_shared<RotCamera>(scale, lookat, presets);
}

}  // namespace g3d
