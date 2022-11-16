#pragma once

#include <memory>
#include <vector>
#include <optional>

#include <glm/gtc/matrix_transform.hpp>

namespace g3d {

struct Object;
struct Camera;

struct Hit {
    Object *object;
    float distance;
    size_t primitive;
    glm::vec3 world_pos;
};

struct Scene {
    virtual ~Scene(){};

    virtual bool prepare() = 0;

    virtual void draw() = 0;

    std::shared_ptr<Camera> m_camera;

    std::vector<std::shared_ptr<Object>> m_objects;

    std::optional<glm::vec3> m_lightpos;

    Hit m_hit;
};

std::shared_ptr<Scene> makeGLFWImguiScene(const char *title, int width,
                                          int height,
                                          float ground_checker_size = 1000);

}  // namespace g3d
