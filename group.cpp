#include "object.hpp"
#include <glm/gtc/type_ptr.hpp>

#include "opengl.hpp"

namespace g3d {

struct Group : public Object {
    Group(const char *name) { m_name = name; }

    void draw(const Scene &scene, const Camera &cam,
              const glm::mat4 &pt) override
    {
        if(!m_visible)
            return;
        for(auto &o : m_children) {
            o->draw(scene, cam, m_model_matrix * pt);
        }
    }

    void ui(const Scene &scene) override
    {
        ImGui::Checkbox("Visible", &m_visible);
        ImGui::Checkbox("Rigid Transform", &m_rigid);

        if(m_rigid) {
            ImGui::SliderFloat3("Translation", glm::value_ptr(m_translation),
                                -100, 100);
            ImGui::Text("Rotation");
            ImGui::SliderAngle("X##r", &m_rotation.x);
            ImGui::SliderAngle("Y##r", &m_rotation.y);
            ImGui::SliderAngle("Z##r", &m_rotation.z);

            auto m = glm::mat4(1);
            m = glm::translate(m, m_translation);
            m = glm::rotate(m, m_rotation.x, {1, 0, 0});
            m = glm::rotate(m, m_rotation.y, {0, 1, 0});
            m = glm::rotate(m, m_rotation.z, {0, 0, 1});
            m_model_matrix = m;
        } else {
            m_model_matrix = glm::mat4{1};
        }
    }

    void addChild(std::shared_ptr<Object> child) override
    {
        m_children.push_back(child);
    }

    glm::vec3 m_translation{0};
    glm::vec3 m_rotation{0};
    glm::vec3 m_bbox_center{0};
    glm::vec3 m_bbox_size{1000};

    bool m_rigid{false};
    bool m_visible{true};

    std::vector<std::shared_ptr<Object>> m_children;
};

std::shared_ptr<Object>
makeGroup(const char *name)
{
    return std::make_shared<Group>(name);
}

}  // namespace g3d
