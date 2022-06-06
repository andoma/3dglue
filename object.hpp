#pragma once

#include <memory>
#include <vector>
#include <string>
#include <optional>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace g3d {

struct Camera;
struct Scene;

struct Object {
    virtual ~Object(){};

    virtual void ui(const Scene &s) {}

    virtual void draw(const Scene &s, const Camera &c) = 0;

    virtual void setColor(const glm::vec4 &ambient,
                          const glm::vec4 &diffuse = glm::vec4{0},
                          const glm::vec4 &specular = glm::vec4{0})
    {
    }

    virtual void addChild(std::shared_ptr<Object> child) {}

    void setModelMatrix(const glm::mat4 &m) { m_model_matrix = m; }

    void setModelMatrixScale(float scale)
    {
        m_model_matrix = glm::scale(glm::mat4{1}, {scale, scale, scale});
    }

    glm::mat4 m_model_matrix{1};

    std::optional<std::string> m_name;

    bool m_visible{true};
};

std::shared_ptr<Object> makePointCloud(size_t num_points, const float *xyz,
                                       const float *rgb = NULL,
                                       const float *trait = NULL);

std::shared_ptr<Object> makeCross();

struct Mesh;

std::shared_ptr<Object> makeMeshObject(const Mesh &data);

std::shared_ptr<Object> makeSkybox();

std::shared_ptr<Object> makeGround(float checkersize);

std::shared_ptr<Object> makeLine(const glm::vec3 segment[2]);

}  // namespace g3d
