#include "object.hpp"

#include "shader.hpp"
#include "vertexbuffer.hpp"
#include "arraybuffer.hpp"
#include "texture.hpp"
#include "camera.hpp"
#include "scene.hpp"
#include "bvh.hpp"

extern unsigned char phong_vertex_glsl[];
extern int phong_vertex_glsl_len;
extern unsigned char phong_geometry_glsl[];
extern int phong_geometry_glsl_len;
extern unsigned char phong_fragment_glsl[];
extern int phong_fragment_glsl_len;

namespace g3d {

struct Mesh : public Object {
    Mesh(const std::shared_ptr<VertexBuffer> &vb,
         const std::shared_ptr<std::vector<glm::ivec3>> &ib, bool interactive)
      : m_vb(vb), m_ib(ib), m_interactive(interactive)
    {
        m_name = "Mesh";
        if(ib)
            m_update_index_buffer = true;
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

    void draw(const Scene &scene, const Camera &cam,
              const glm::mat4 &pt) override
    {
        if(m_update_index_buffer) {
            m_update_index_buffer = false;
            m_elements = m_ib->size();
            m_drawcount = m_elements;
            m_index_buf.write((void *)m_ib->data(),
                              m_ib->size() * sizeof(glm::ivec3));
        }

        if(m_vb) {
            if(m_interactive && m_ib) {
                m_intersector = Intersector::make(m_vb, m_ib);
            }

            uint32_t mask = m_vb->get_attribute_mask();

            // Recompile shader if attribute setup changes
            if(m_attrib_buf.get_attribute_mask() ^ mask) {
                char hdr[4096];

                snprintf(hdr, sizeof(hdr),
                         "#version 330 core\n"
                         "%s%s%s",
                         m_vb->get_elements(VertexAttribute::Normal)
                             ? "#define PER_VERTEX_NORMAL\n"
                             : "",
                         m_vb->get_elements(VertexAttribute::Color)
                             ? "#define PER_VERTEX_COLOR\n"
                             : "",
                         m_vb->get_elements(VertexAttribute::UV0)
                             ? "#define TEX0\n"
                             : "");

                // clang-format off
                m_shader = std::make_unique<Shader>("phong",
            hdr,
            (const char *)phong_vertex_glsl,   (int)phong_vertex_glsl_len,
            (const char *)phong_fragment_glsl, (int)phong_fragment_glsl_len,
            (const char *)phong_geometry_glsl, (int)phong_geometry_glsl_len);
                // clang-format on
            }

            m_attrib_buf.load(*m_vb);
            m_vb.reset();
        }

        if(!m_attrib_buf.bind())
            return;

        Shader *s = m_shader.get();

        s->use();

        auto m = pt * m_model_matrix;
        if(m_rigid)
            m = m_edit_matrix * m;

        s->setMat4("PVM", cam.m_P * cam.m_V * m);
        if(s->has_uniform("M")) {
            s->setMat4("M", m);
        }

        if(m_attrib_buf.get_elements(VertexAttribute::Color)) {
            s->setFloat("per_vertex_color_blend", m_colorize);
        }

        if(scene.m_lightpos) {
            s->setVec3("lightPos", *scene.m_lightpos);
            s->setVec3("diffuseColor", m_diffuse);
            s->setVec3("specularColor", m_specular);
            s->setVec3("ambientColor", m_ambient);
        } else {
            s->setVec3("ambientColor", glm::vec3{1.0f});
        }

        s->setFloat("normalColorize", m_normal_colorize);
        s->setFloat("alpha", m_alpha);

        if(s->has_uniform("viewPos")) {
            s->setVec3("viewPos", glm::vec3{cam.m_VI[3]});
        }

        glEnableVertexAttribArray(0);
        m_attrib_buf.ptr(0, VertexAttribute::Position);

        if(m_attrib_buf.get_elements(VertexAttribute::Normal)) {
            glEnableVertexAttribArray(1);
            m_attrib_buf.ptr(1, VertexAttribute::Normal);
        }

        if(m_attrib_buf.get_elements(VertexAttribute::Color)) {
            glEnableVertexAttribArray(2);
            m_attrib_buf.ptr(2, VertexAttribute::Normal);
        }

        if(m_attrib_buf.get_elements(VertexAttribute::UV0)) {
            glEnableVertexAttribArray(3);
            m_attrib_buf.ptr(3, VertexAttribute::UV0);
        }

        GLuint tex0 = m_tex0.get();
        if(tex0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex0);
            s->setInt("tex0", 0);
        }

        if(m_backface_culling)
            glEnable(GL_CULL_FACE);

        if(m_wireframe)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        if(m_index_buf.bind()) {
            glDrawElements(GL_TRIANGLES, m_drawcount * 3, GL_UNSIGNED_INT,
                           NULL);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, m_attrib_buf.size());
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_CULL_FACE);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(3);
    }

    void ui(const Scene &scene) override
    {
        ImGui::Checkbox("Visible", &m_visible);
        ImGui::Checkbox("Wireframe", &m_wireframe);
        ImGui::Checkbox("Backface culling", &m_backface_culling);

        ImGui::SliderFloat("Color", &m_colorize, 0, 1);
        ImGui::SliderFloat("Alpha", &m_alpha, 0, 1);

        if(scene.m_lightpos) {
            ImGui::SliderFloat3("Ambient", &m_ambient[0], 0, 1);
            ImGui::SliderFloat3("Diffuse", &m_diffuse[0], 0, 1);
            ImGui::SliderFloat3("Specular", &m_specular[0], 0, 1);
        }
        ImGui::SliderFloat("Normals", &m_normal_colorize, 0, 1);

        if(m_elements) {
            ImGui::SliderInt("DrawCount", &m_drawcount, 0, m_elements);
        }

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
    }

    void setColor(const glm::vec4 &ambient, const glm::vec4 &diffuse,
                  const glm::vec4 &specular) override
    {
        m_ambient = ambient;
        m_diffuse = diffuse;
        m_specular = specular;
    }

    void set(const std::shared_ptr<Image2D> &img) override { m_tex0.set(img); }

    void set(const std::string &key, float val) override
    {
        if(key == "vertexcolor")
            m_colorize = val;
        if(key == "normalcolors")
            m_normal_colorize = val;
    }

    void set(const std::shared_ptr<VertexBuffer> &vb) override { m_vb = vb; }

    VertexAttribBuffer m_attrib_buf;
    ArrayBuffer m_index_buf{GL_ELEMENT_ARRAY_BUFFER};

    std::unique_ptr<Shader> m_shader;
    int m_drawcount{0};
    int m_elements{0};

    glm::vec3 m_ambient{1};
    glm::vec3 m_specular{1};
    glm::vec3 m_diffuse{1};

    float m_colorize{1};
    float m_alpha{1};
    float m_normal_colorize{0};

    bool m_wireframe{false};
    bool m_backface_culling{true};

    Texture2D m_tex0;

    std::shared_ptr<VertexBuffer> m_vb;
    std::shared_ptr<std::vector<glm::ivec3>> m_ib;
    const bool m_interactive;

    bool m_rigid{false};
    glm::vec3 m_translation{0};
    glm::vec3 m_rotation{0};
    glm::mat4 m_edit_matrix{1};

    std::shared_ptr<Intersector> m_intersector;

    bool m_update_index_buffer{false};
};

std::shared_ptr<Object>
makeMesh(const std::shared_ptr<VertexBuffer> &vb,
         const std::vector<glm::ivec3> &ib, bool interactive)
{
    auto ibsp = std::make_shared<std::vector<glm::ivec3>>(ib);
    return std::make_shared<Mesh>(vb, ibsp, interactive);
}

}  // namespace g3d
