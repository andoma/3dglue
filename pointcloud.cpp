#include "object.hpp"

#include "arraybuffer.hpp"
#include "shader.hpp"
#include "camera.hpp"
#include "scene.hpp"
#include "bvh.hpp"

static const char *pc_vertex_shader = R"glsl(
layout (location = 0) in vec3 aPos;

#ifdef PER_VERTEX_COLOR
layout (location = 1) in vec4 aCol;
#endif

#ifdef PER_VERTEX_TRAIT
layout (location = 2) in float aTrait;
#endif

uniform mat4 PV;
uniform mat4 model;
uniform vec4 albedo;
uniform vec3 bbox1;
uniform vec3 bbox2;
uniform float alpha;
uniform int focused;
uniform int pointsize;
uniform vec2 trait_minmax;

out vec4 fragmentColor;

void main()
{
   gl_Position = PV * model * vec4(aPos.xyz, 1);

   vec3 s = step(bbox1, aPos.xyz) - step(bbox2, aPos.xyz);
   float inside = s.x * s.y * s.z;

   float a = alpha * (inside + 0.25);
#ifdef PER_VERTEX_TRAIT
   a = a * (aTrait >= trait_minmax.x && aTrait <= trait_minmax.y ? 1.0 : 0.0);
#endif

   if(gl_VertexID == focused) {
     gl_PointSize = 10;
   } else {
     gl_PointSize = pointsize;
   }

#ifdef PER_VERTEX_COLOR
   fragmentColor = vec4(aCol.xyz, a) * albedo;
#else
   fragmentColor = vec4(1,1,1, a) * albedo;
#endif


}

)glsl";

static const char *pc_fragment_shader = R"glsl(
out vec4 FragColor;
in vec4 fragmentColor;

void main()
{
  if(fragmentColor.a == 0.0) {
    discard;
  } else {
    FragColor = fragmentColor;
  }
}

)glsl";

namespace g3d {

struct PointCloud : public Object {
    std::unique_ptr<Shader> m_shader;

    VertexAttribBuffer m_attrib_buf;

    PointCloud(const std::shared_ptr<VertexBuffer> &vb, bool interactive)
      : m_vb(vb), m_interactive(interactive)
    {
        m_name = "Pointcloud";
    }

    void hit(const glm::vec3 &origin, const glm::vec3 &direction,
             const glm::mat4 &parent_mm, Hit &hit) override
    {
        if(!m_intersector)
            return;

        auto m = parent_mm * m_model_matrix;
        if(m_rigid)
            m = m_edit_matrix * m;

        const auto m_I = glm::inverse(m);
        const auto o = m_I * glm::vec4(origin, 1);
        const auto dir = glm::normalize(m_I * glm::vec4(direction, 0));
        const auto res = m_intersector->intersect(o, dir);

        if(res) {
            auto p = m * glm::vec4(res->second, 1);
            auto d = glm::distance(glm::vec3(p), origin);

            if(d < hit.distance) {
                hit.object = this;
                hit.primitive = res->first;
                hit.distance = d;
                hit.world_pos = p;
            }
        }
    }

    void set(const std::shared_ptr<VertexBuffer> &vb) override { m_vb = vb; }

    void draw(const Scene &scene, const Camera &cam,
              const glm::mat4 &pt) override
    {
        if(m_vb) {
            if(m_interactive)
                m_intersector =
                    Intersector::make(m_vb, IntersectionMode::POINT);

            uint32_t mask = m_vb->get_attribute_mask();
            // Recompile shader if attribute setup changes
            if(m_attrib_buf.get_attribute_mask() ^ mask) {
                char hdr[4096];
                snprintf(hdr, sizeof(hdr),
                         "#version 330 core\n"
                         "%s%s",
                         m_vb->get_elements(VertexAttribute::Color)
                             ? "#define PER_VERTEX_COLOR\n"
                             : "",
                         m_vb->get_elements(VertexAttribute::Aux)
                             ? "#define PER_VERTEX_TRAIT\n"
                             : "");
                m_shader = std::make_unique<Shader>("pointcloud", hdr,
                                                    pc_vertex_shader, -1,
                                                    pc_fragment_shader, -1);
            }

            m_attrib_buf.load(*m_vb);
            m_vb.reset();
        }

        if(!m_attrib_buf.bind())
            return;

        Shader *s = m_shader.get();
        s->use();
        s->setMat4("PV", cam.m_P * cam.m_V);

        auto m = pt * m_model_matrix;
        if(m_rigid)
            m = m_edit_matrix * m;

        s->setMat4("model", m);
        s->setVec4("albedo", glm::vec4{glm::vec3{m_color}, 1});
        s->setFloat("alpha", m_alpha);

        if(m_attrib_buf.get_elements(VertexAttribute::Aux)) {
            if(m_trait_on) {
                s->setVec2("trait_minmax", glm::vec2{m_trait_min, m_trait_max});
            } else {
                s->setVec2("trait_minmax", glm::vec2{-INFINITY, INFINITY});
            }
        }
        if(m_bb) {
            s->setVec3("bbox1", m_bbox_center - m_bbox_size * 0.5f);
            s->setVec3("bbox2", m_bbox_center + m_bbox_size * 0.5f);
        } else {
            s->setVec3("bbox1", glm::vec3{-INFINITY});
            s->setVec3("bbox2", glm::vec3{INFINITY});
        }

        s->setInt("focused",
                  scene.m_hit.object == this ? scene.m_hit.primitive : -1);

        s->setInt("pointsize", m_pointsize);

        glEnableVertexAttribArray(0);

        m_attrib_buf.ptr(0, VertexAttribute::Position);

        if(m_attrib_buf.get_elements(VertexAttribute::Color)) {
            glEnableVertexAttribArray(1);
            m_attrib_buf.ptr(1, VertexAttribute::Color);
        }

        if(m_attrib_buf.get_elements(VertexAttribute::Aux)) {
            glEnableVertexAttribArray(2);
            m_attrib_buf.ptr(2, VertexAttribute::Aux);
        }
        glEnable(GL_PROGRAM_POINT_SIZE);
        glDrawArrays(GL_POINTS, 0, m_attrib_buf.size());
        glDisable(GL_PROGRAM_POINT_SIZE);
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
    }

    void setColor(const glm::vec4 &ambient, const glm::vec4 &diffuse,
                  const glm::vec4 &specular) override
    {
        m_color = ambient;
    }

    void set(const std::string &key, float val) override
    {
        if(key == "pointsize")
            m_pointsize = val;
    }

    void ui(const Scene &scene) override
    {
        ImGui::Checkbox("Visible", &m_visible);

        ImGui::Text("%zd points", m_attrib_buf.size());
        ImGui::SliderInt("PointSize", &m_pointsize, 1, 10);

        ImGui::SliderFloat("Alpha", &m_alpha, 0, 1);

        ImGui::Checkbox("Rigid Transform", &m_rigid);

        if(m_rigid) {
            ImGui::Text("Translation");
            ImGui::SliderFloat("X##t", &m_translation.x, -50, 50);
            ImGui::SliderFloat("Y##t", &m_translation.y, -50, 50);
            ImGui::SliderFloat("Z##t", &m_translation.z, -50, 50);

            ImGui::Text("Rotation");
            ImGui::SliderAngle("X##r", &m_rotation.x);
            ImGui::SliderAngle("Y##r", &m_rotation.y);
            ImGui::SliderAngle("Z##r", &m_rotation.z);

            auto m = glm::mat4(1);
            m = glm::translate(m, m_translation);
            m = glm::rotate(m, m_rotation.x, {1, 0, 0});
            m = glm::rotate(m, m_rotation.y, {0, 1, 0});
            m = glm::rotate(m, m_rotation.z, {0, 0, 1});
            m_edit_matrix = m;
        }

        ImGui::Checkbox("BoundingBox", &m_bb);
        if(m_bb) {
            ImGui::Text("Position");
            ImGui::SliderFloat("X##c", &m_bbox_center.x, -5000, 5000);
            ImGui::SliderFloat("Y##c", &m_bbox_center.y, -5000, 5000);
            ImGui::SliderFloat("Z##c", &m_bbox_center.z, -5000, 5000);
            ImGui::Text("Size");
            ImGui::SliderFloat("X##s", &m_bbox_size.x, 1, 5000);
            ImGui::SliderFloat("Y##s", &m_bbox_size.y, 1, 5000);
            ImGui::SliderFloat("Z##s", &m_bbox_size.z, 1, 5000);
        }

        ImGui::Checkbox("TraitRange", &m_trait_on);
        if(m_trait_on) {
            ImGui::SliderFloat("Min", &m_trait_min, 0, 1);
            ImGui::SliderFloat("Max", &m_trait_max, 0, 1);
        }
    }

    std::shared_ptr<VertexBuffer> m_vb;

    glm::vec4 m_color{1};

    glm::vec3 m_translation{0};
    glm::vec3 m_rotation{0};
    glm::vec3 m_bbox_center{0};
    glm::vec3 m_bbox_size{1000};

    bool m_rigid{false};
    bool m_bb{false};
    float m_alpha{1};
    int m_pointsize{1};

    bool m_trait_on{false};
    float m_trait_min{0};
    float m_trait_max{1};

    std::shared_ptr<Intersector> m_intersector;
    const bool m_interactive{false};

    glm::mat4 m_edit_matrix{1};
};

std::shared_ptr<Object>
makePointCloud(const std::shared_ptr<VertexBuffer> &vb, bool interactive)
{
    return std::make_shared<PointCloud>(vb, interactive);
}

}  // namespace g3d
