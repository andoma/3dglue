#include <iostream>

#include "camera.hpp"
#include "opengl.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <glm/gtx/matrix_decompose.hpp>

namespace g3d {

struct ArcBallCamera : public Camera {
    ArcBallCamera(glm::vec3 lookat, glm::vec3 position,
                  const std::map<std::string, glm::mat4> &presets)
      : m_lookat(lookat), m_presets(presets)
    {
        init_from_position(position);
    }

    void init_from_position(glm::vec3 position)
    {
        auto m =
            glm::inverse(glm::lookAt(position, m_lookat, glm::vec3{0, 0, 1}));

        glm::vec3 scale;
        glm::quat orientation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;

        glm::decompose(m, scale, orientation, translation, skew, perspective);

        m_rotation = orientation;
        m_distance = glm::length(glm::length(position - m_lookat));
    }

    glm::vec3 lookAt() const override { return m_lookat; }

    glm::vec3 position() const override
    {
        return glm::vec3{0};  // FIXME
    }

    glm::mat4 compute() override
    {
        auto T0 = glm::translate(glm::mat4(1.0f), glm::vec3{0, 0, m_distance});
        auto R = glm::mat4_cast(m_rotation);
        auto T1 = glm::translate(glm::mat4(1.0f), m_lookat);

        auto V = T1 * R * T0;
        m_position = V[3];

        auto view = glm::inverse(V);

        glm::vec3 scale;
        glm::quat orientation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;

        glm::decompose(glm::rotate(view, (float)(M_PI / 2), glm::vec3{1,0,0}),
                       scale, orientation, translation, skew, perspective);

        auto euler = glm::eulerAngles(orientation) * (float)(180.0f / M_PI);

        ImGui::Text("Pivot point");
        ImGui::Indent();
        ImGui::Text("X:% -9.2f Y:% -9.2f Z:% -9.2f",
                    m_lookat.x, m_lookat.y, m_lookat.z);
        ImGui::Unindent();
        ImGui::Text("Camera");
        ImGui::Indent();
        ImGui::Text("X:% -9.2f Y:% -9.2f Z:% -9.2f",
                    m_position.x, m_position.y, m_position.z);
        ImGui::Text("Distance:%9.2f", m_distance);
        ImGui::Text("Yaw:% 6.1f°  Pitch:% 6.1f°  Roll:% 6.1f°",
                    euler.x, euler.y, euler.z);
        ImGui::Unindent();
        return view;
    }

    void uiInput(Control c, const glm::vec2 &xy) override
    {
        glm::quat q;

        switch(c) {
        default:
            break;

        case Control::SCROLL:
            m_rotation *=
                glm::angleAxis(glm::radians(xy.x), glm::vec3(0.f, 1.0f, 0.f));
            if(xy.y < 0) {
                m_distance *= powf(1.1, -xy.y);
            } else {
                m_distance *= powf(0.9, xy.y);
            }
            break;

        case Control::DRAG1:
            q = glm::angleAxis(glm::radians(xy.x * -90),
                               glm::vec3(0.f, 1.0f, 0.f));

            q *= glm::angleAxis(glm::radians(xy.y * -90),
                                glm::vec3(1.f, 0.0f, 0.f));

            m_rotation *= q;
            break;

        case Control::DRAG2:
            q = glm::angleAxis(glm::radians(xy.x * -90),
                               glm::vec3(0.f, 0.0f, 1.f));

            q *= glm::angleAxis(glm::radians(xy.y * -90),
                                glm::vec3(1.f, 0.0f, 0.f));
            m_rotation *= q;
            break;

        case Control::DRAG3:
            m_lookat += glm::vec3(
                glm::mat4_cast(m_rotation) *
                glm::vec4{-m_distance * xy.x, m_distance * xy.y, 0, 1});
            break;

        case Control::DRAG4:
            m_lookat += glm::vec3(
                glm::mat4_cast(m_rotation) *
                glm::vec4{-m_distance * xy.x, 0, m_distance * xy.y, 1});
            break;
        }
    }

    void select(const glm::mat4 &m)
    {
        /*
                auto pos = m[3];
                auto dir = glm::normalize(m[1]);  // Positive Y

                auto lookat = pos + dir * 1000.0f;

                auto r = pos - lookat;
                m_lookat = lookat;
                m_height = r.z;
                m_azimuth = atan2f(-r.x, -r.y);
                m_distance = -sqrtf(r.x * r.x + r.y * r.y);
        */
    }

    void select(const std::string &preset) override
    {
        /*
            auto it = m_presets.find(preset);
            if(it != m_presets.end())
                select(it->second);
            */
    }

    void positionStore(int slot) override
    {
        /*
        char name[PATH_MAX];
        snprintf(name, sizeof(name), ".rotcam%d", slot);
        FILE *fp = fopen(name, "w");
        fprintf(fp, "%f %f %f %f %f %f\n", m_distance, m_height, m_azimuth,
                m_lookat.x, m_lookat.y, m_lookat.z);
        fclose(fp);
        */
    }

    void positionRecall(int slot) override
    {
        /*
        char name[PATH_MAX];
        snprintf(name, sizeof(name), ".rotcam%d", slot);
        FILE *fp = fopen(name, "r");
        if(fp) {
            if(fscanf(fp, "%f %f %f %f %f %f\n", &m_distance, &m_height,
                      &m_azimuth, &m_lookat.x, &m_lookat.y, &m_lookat.z)) {
            }
            fclose(fp);
        }
        */
    }

    glm::vec3 m_lookat;
    glm::quat m_rotation;
    float m_distance;
    glm::vec3 m_position;

    const std::map<std::string, glm::mat4> m_presets;
};

std::shared_ptr<Camera>
makeArcBallCamera(glm::vec3 lookat, glm::vec3 position,
                  const std::map<std::string, glm::mat4> &presets)
{
    return std::make_shared<ArcBallCamera>(lookat, position, presets);
}

}  // namespace g3d
