#include <memory>
#include <string>
#include <map>

#include <glm/glm.hpp>

#include "object.hpp"

namespace g3d {

struct Camera {
    virtual ~Camera(){};

    virtual void ui(){};

    virtual void update(float viewport_width, float viewport_height) = 0;

    virtual float distance() const = 0;

    virtual glm::vec3 lookAt() const = 0;

    glm::vec3 position() const { return m_VI[3]; }

    virtual void positionStore(int slot){};

    virtual void positionRecall(int slot){};

    virtual void uiInput(Control c, const glm::vec2 &xy){};

    virtual void select(const std::string &preset){};

    virtual void lookat(const glm::vec3 &v){};

    virtual glm::quat orientation() const = 0;

    float m_fov{45};

    glm::mat4 m_P{1};  // Projection

    glm::mat4 m_Pz0{1};  // Projection

    glm::mat4 m_V{1};  // View

    glm::mat4 m_VI{1};  // View inverse
};

std::shared_ptr<Camera> makeArcBallCamera(
    glm::vec3 lookat, glm::vec3 position, float fov, float znear, float zfar,
    const std::map<std::string, glm::mat4> &presets = {});

std::shared_ptr<Camera> makeFixedCamera(const glm::mat4 &extr_matrix, float fov,
                                        float znear, float zfar);

std::shared_ptr<Camera> makeFixedCamera(const glm::vec3 &eye,
                                        const glm::vec3 &center,
                                        const glm::vec3 &up, float fov,
                                        float znear, float zfar);

}  // namespace g3d
