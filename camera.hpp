#include <memory>
#include <string>
#include <map>

#include <glm/glm.hpp>

namespace g3d {

struct Camera {
    enum class Control {
        DRAG1,
        DRAG2,
        DRAG3,
        DRAG4,
        SCROLL,
    };

    virtual ~Camera(){};

    virtual glm::mat4 compute() = 0;

    virtual glm::vec3 lookAt() const = 0;

    virtual glm::vec3 position() const = 0;

    virtual void positionStore(int slot){};

    virtual void positionRecall(int slot){};

    virtual void uiInput(Control c, const glm::vec2 &xy){};

    virtual void select(const std::string &preset){};
};

std::shared_ptr<Camera> makeArcBallCamera(
    glm::vec3 lookat, glm::vec3 position,
    const std::map<std::string, glm::mat4> &presets = {});

}  // namespace g3d
