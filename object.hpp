#pragma once

#include <memory>
#include <vector>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace g3d {

struct Object {
    virtual ~Object(){};

    virtual void prepare() {}

    virtual void draw(const glm::mat4 &P, const glm::mat4 &V) = 0;

    virtual void setColor(const glm::vec4 &) {}

    void setModelMatrix(const glm::mat4 &m) { m_model_matrix = m; }

    void setModelMatrixScale(float scale)
    {
        m_model_matrix = glm::scale(glm::mat4{1}, {scale, scale, scale});
    }

    glm::mat4 m_model_matrix{1};

    std::string m_name;

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
