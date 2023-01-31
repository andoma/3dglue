#include <iostream>

#include "camera.hpp"
#include "opengl.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <glm/gtx/matrix_decompose.hpp>

#include <sys/stat.h>
#include <sys/types.h>

namespace g3d {

glm::vec3
Camera::direction(const glm::vec2 &xy) const
{
    auto VPI = glm::inverse(m_P * m_V);
    auto screenpos = glm::vec4(xy.x, -xy.y, 1.0f, 1.0f);
    auto worldpos = VPI * screenpos;
    return glm::normalize(glm::vec3(worldpos));
}

struct PerspectiveCamera : public Camera {
    PerspectiveCamera(float fov, float znear, float zfar)
      : m_fov(fov), m_znear(znear), m_zfar(zfar)
    {
    }

    virtual void ui() override { ImGui::SliderFloat("FOV", &m_fov, 1, 180); }

    virtual void update(float viewport_width, float viewport_height) override
    {
        m_P =
            glm::perspective(glm::radians(m_fov),
                             viewport_width / viewport_height, m_znear, m_zfar);
    }

    glm::quat rotation() const override
    {
        glm::vec3 scale;
        glm::quat orientation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;

        // Adjust for z-is-up
        // This generates an identity quat if we look straight down the
        // positive Y axis
        auto m = glm::rotate(m_VI, (float)(M_PI / 2), glm::vec3{1, 0, 0});
        glm::decompose(m, scale, orientation, translation, skew, perspective);
        return orientation;
    }

    float m_fov;
    const float m_znear;
    const float m_zfar;
};

struct ArcBallCamera : public PerspectiveCamera {
    ArcBallCamera(glm::vec3 lookat, glm::vec3 position, float fov, float znear,
                  float zfar, const std::map<std::string, glm::mat4> &presets)
      : PerspectiveCamera(fov, znear, zfar)
      , m_lookat(lookat)
      , m_presets(presets)
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

        compute_view();
    }

    glm::vec3 lookAt() const override { return m_lookat; }

    void compute_view(void)
    {
        auto T0 = glm::translate(glm::mat4(1.0f), glm::vec3{0, 0, m_distance});
        auto R = glm::mat4_cast(m_rotation);
        auto T1 = glm::translate(glm::mat4(1.0f), m_lookat);

        m_VI = T1 * R * T0;
        m_V = glm::inverse(m_VI);
    }

    void ui() override
    {
        PerspectiveCamera::ui();

        auto position = m_VI[3];

        glm::vec3 scale;
        glm::quat orientation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;

        glm::decompose(glm::rotate(m_V, (float)(M_PI / 2), glm::vec3{1, 0, 0}),
                       scale, orientation, translation, skew, perspective);

        auto euler = glm::eulerAngles(orientation) * (float)(180.0f / M_PI);

        ImGui::InputFloat3("Pivot", glm::value_ptr(m_lookat));

        ImGui::Text("Camera");
        ImGui::Indent();
        ImGui::Text("X:% -9.2f Y:% -9.2f Z:% -9.2f", position.x, position.y,
                    position.z);
        ImGui::Text("Distance:%9.2f", m_distance);
        ImGui::Text("Pitch:% 6.1f°  Yaw:% 6.1f°  Roll:% 6.1f°", euler.x,
                    euler.y, euler.z);
        ImGui::Checkbox("AutoRotate", &m_autorotate);
        ImGui::Unindent();

        if(m_presets.size()) {
            if(ImGui::Button("Goto")) {
                ImGui::OpenPopup("Goto");
            }

            if(ImGui::BeginPopup("Goto")) {
                for(const auto &p : m_presets) {
                    if(ImGui::Button(p.first.c_str())) {
                        ImGui::CloseCurrentPopup();
                        set(p.second);
                        break;
                    }
                }
                ImGui::EndPopup();
            }
        }
    }

    void update(float viewport_width, float viewport_height) override
    {
        if(m_autorotate) {
            auto q =
                glm::angleAxis(glm::radians(0.5f), glm::vec3(0.f, 1.0f, 0.f));
            m_rotation *= q;
        }
        PerspectiveCamera::update(viewport_width, viewport_height);
        compute_view();
    }

    float distance() const override { return m_distance; }

    void uiInput(Control c, const glm::vec2 &xy) override
    {
        glm::quat q;

        switch(c) {
        default:
            break;

        case Control::ZOOM:
            if(xy.y < 0) {
                m_distance *= powf(1.1, -xy.y);
            } else {
                m_distance *= powf(0.9, xy.y);
            }
            break;

        case Control::ROTATE1:
            q = glm::angleAxis(glm::radians(xy.x * -90),
                               glm::vec3(0.f, 1.0f, 0.f));

            q *= glm::angleAxis(glm::radians(xy.y * -90),
                                glm::vec3(1.f, 0.0f, 0.f));

            m_rotation *= q;
            break;

        case Control::ROTATE2:
            q = glm::angleAxis(glm::radians(xy.x * -90),
                               glm::vec3(0.f, 0.0f, 1.f));

            q *= glm::angleAxis(glm::radians(xy.y * -90),
                                glm::vec3(1.f, 0.0f, 0.f));
            m_rotation *= q;
            break;

        case Control::TRANSLATE1:
            m_lookat +=
                glm::vec3(glm::mat3_cast(m_rotation) *
                          glm::vec3{-m_distance * xy.x, m_distance * xy.y, 0});
            break;

        case Control::TRANSLATE2:
            m_lookat +=
                glm::vec3(glm::mat3_cast(m_rotation) *
                          glm::vec3{-m_distance * xy.x, 0, m_distance * xy.y});
            break;
        }
    }

    void lookat(const glm::vec3 &v) override { m_lookat = v; }

    void set(const glm::mat4 &m)
    {
        auto pos = m[3];
        auto dir = glm::normalize(m[1]);  // Positive Y

        m_lookat = pos + dir * 1000.0f;
        init_from_position(pos);
    }

    void select(const std::string &preset) override
    {
        auto it = m_presets.find(preset);
        if(it != m_presets.end())
            set(it->second);
    }

    void transformStore(int slot) override
    {
        mkdir(".3dglue", 0777);

        char name[PATH_MAX];
        snprintf(name, sizeof(name), ".3dglue/arcballcam%d", slot);
        FILE *fp = fopen(name, "w");
        fprintf(fp, "%f %f %f %f %f %f %f %f\n", m_distance, m_lookat.x,
                m_lookat.y, m_lookat.z, m_rotation.x, m_rotation.y,
                m_rotation.z, m_rotation.w);
        fclose(fp);
    }

    void transformRecall(int slot) override
    {
        char name[PATH_MAX];
        snprintf(name, sizeof(name), ".3dglue/arcballcam%d", slot);
        FILE *fp = fopen(name, "r");
        if(fp == NULL)
            return;

        if(fscanf(fp, "%f %f %f %f %f %f %f %f\n", &m_distance, &m_lookat.x,
                  &m_lookat.y, &m_lookat.z, &m_rotation.x, &m_rotation.y,
                  &m_rotation.z, &m_rotation.w)) {
        }
        fclose(fp);
    }

    glm::vec3 m_lookat;
    glm::quat m_rotation;
    float m_distance;
    bool m_autorotate{false};
    const std::map<std::string, glm::mat4> m_presets;
};

std::shared_ptr<Camera>
makeArcBallCamera(glm::vec3 lookat, glm::vec3 position, float fov, float znear,
                  float zfar, const std::map<std::string, glm::mat4> &presets)
{
    return std::make_shared<ArcBallCamera>(lookat, position, fov, znear, zfar,
                                           presets);
}

struct FixedCamera : public PerspectiveCamera {
    FixedCamera(const glm::mat4 &extr_matrix, float fov, float znear,
                float zfar)
      : PerspectiveCamera(fov, znear, zfar)
    {
        m_VI = extr_matrix;
        m_V = glm::inverse(extr_matrix);
    }

    glm::vec3 lookAt() const override { return glm::vec3{0}; }

    float distance() const override { return NAN; }
};

std::shared_ptr<Camera>
makeFixedCamera(const glm::mat4 &extr_matrix, float fov, float znear,
                float zfar)
{
    return std::make_shared<FixedCamera>(extr_matrix, fov, znear, zfar);
}

std::shared_ptr<Camera>
makeFixedCamera(const glm::vec3 &eye, const glm::vec3 &center,
                const glm::vec3 &up, float fov, float znear, float zfar)
{
    return std::make_shared<FixedCamera>(
        glm::inverse(glm::lookAt(eye, center, up)), fov, znear, zfar);
}

}  // namespace g3d
