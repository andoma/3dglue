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
struct VertexBuffer;
struct IndexBuffer;
struct Image2D;
struct Object;

struct Hit {
    Object *object;
    float distance;
    size_t primitive;
    glm::vec3 world_pos;
};

enum class Control {

    ROTATE_START,
    ROTATE1,
    ROTATE2,
    ROTATE_END,

    TRANSLATE_START,
    TRANSLATE1,
    TRANSLATE2,
    TRANSLATE_END,

    ZOOM,
};

struct Object : public std::enable_shared_from_this<Object> {
    virtual ~Object(){};

    virtual void ui(const Scene &s) {}

    virtual void draw(const Scene &s, const Camera &c,
                      const glm::mat4 &parent_mm) = 0;

    virtual void hit(const glm::vec3 &origin, const glm::vec3 &direction,
                     const glm::mat4 &parent_mm, Hit &hit)
    {
    }

    virtual void setColor(const glm::vec4 &ambient,
                          const glm::vec4 &diffuse = glm::vec4{0},
                          const glm::vec4 &specular = glm::vec4{0})
    {
    }

    virtual void addChild(std::shared_ptr<Object> child) {}

    virtual void set(const std::shared_ptr<VertexBuffer> &vb) {}

    virtual void set(const std::shared_ptr<Image2D> &tex) {}

    virtual void set(const std::string &key, float val) {}

    void setModelMatrix(const glm::mat4 &m) { m_model_matrix = m; }

    void setModelMatrixScale(float scale)
    {
        m_model_matrix = glm::scale(glm::mat4{1}, {scale, scale, scale});
    }

    virtual bool uiInput(const Hit &hit, Control c, const glm::vec2 &delta,
                         const Camera &cam, const glm::vec2 &xy)
    {
        return false;
    }

    glm::mat4 m_model_matrix{1};

    std::optional<std::string> m_name;

    bool m_visible{true};
};

std::shared_ptr<Object> makePointCloud(const std::shared_ptr<VertexBuffer> &vb,
                                       bool interactive);

std::shared_ptr<Object> makeCross();

std::shared_ptr<Object> makeMesh(const std::shared_ptr<VertexBuffer> &vb,
                                 const std::vector<glm::ivec3> &ib,
                                 bool interactive = false);

std::shared_ptr<Object> makeSkybox();

std::shared_ptr<Object> makeGround(float checkersize);

std::shared_ptr<Object> makeLine(const glm::vec3 &p1, const glm::vec3 &p2);

std::shared_ptr<Object> makeLine(const glm::vec3 segment[2]);

std::shared_ptr<Object> makeLines(const std::vector<glm::vec3> &lines);

std::shared_ptr<Object> makeLineStrip(const std::vector<glm::vec3> &lines);

std::shared_ptr<Object> makeLines(const std::shared_ptr<VertexBuffer> &vb,
                                  const std::vector<glm::ivec2> &ib);

std::shared_ptr<Object> makeGroup(const char *name);

std::pair<std::shared_ptr<VertexBuffer>,
          std::shared_ptr<std::vector<glm::ivec3>>>
loadOBJ(const char *path, const glm::mat4 transform = glm::mat4{1});

std::shared_ptr<VertexBuffer> loadPCD(const char *path,
                                      const glm::mat4 transform = glm::mat4{1},
                                      glm::vec3 bbmin = {-INFINITY,-INFINITY,-INFINITY},
                                      glm::vec3 bbmax = {INFINITY,INFINITY,INFINITY});

}  // namespace g3d
