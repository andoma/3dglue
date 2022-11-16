#include "object.hpp"
#include <glm/gtc/type_ptr.hpp>

#include "opengl.hpp"

namespace g3d {

struct Group : public Object {
    Group(const char *name) { m_name = name; }

    void hit(const glm::vec3 &origin, const glm::vec3 &direction,
             const glm::mat4 &parent_mm, Hit &hit) override
    {
        for(auto &o : m_children) {
            if(o->m_visible) {
                o->hit(origin, direction, m_model_matrix * parent_mm, hit);
            }
        }
    }

    void draw(const Scene &scene, const Camera &cam,
              const glm::mat4 &parent_mm) override
    {
        for(auto &o : m_children) {
            if(o->m_visible) {
                o->draw(scene, cam, m_model_matrix * parent_mm);
            }
        }
    }

    void ui(const Scene &scene) override
    {
        ImGui::Checkbox("Visible", &m_visible);
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

    std::vector<std::shared_ptr<Object>> m_children;
};

std::shared_ptr<Object>
makeGroup(const char *name)
{
    return std::make_shared<Group>(name);
}

}  // namespace g3d
