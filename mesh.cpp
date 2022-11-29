#include "object.hpp"

#include "shader.hpp"
#include "vertexbuffer.hpp"
#include "arraybuffer.hpp"
#include "mesh.hpp"
#include "texture.hpp"
#include "camera.hpp"
#include "scene.hpp"

extern unsigned char phong_vertex_glsl[];
extern int phong_vertex_glsl_len;
extern unsigned char phong_geometry_glsl[];
extern int phong_geometry_glsl_len;
extern unsigned char phong_fragment_glsl[];
extern int phong_fragment_glsl_len;

namespace g3d {

struct Mesh : public Object {
    Mesh(const std::shared_ptr<VertexBuffer> &vb,
         const std::shared_ptr<std::vector<glm::ivec3>> &ib)
      : m_vb(vb), m_ib(ib)
    {
        m_name = "Mesh";
    }

    void draw(const Scene &scene, const Camera &cam,
              const glm::mat4 &pt) override
    {
        if(m_ib) {
            m_elements = m_ib->size();
            m_drawcount = m_elements;
            m_index_buf.write((void *)m_ib->data(),
                              m_ib->size() * sizeof(glm::ivec3));
            m_ib.reset();
        }

        if(m_vb) {
            uint32_t mask = m_vb->get_attribute_mask();

            // Recompile shader if attribute setup changes
            if(m_attrib_buf.get_attribute_mask() ^ mask) {
                char hdr[4096];

                snprintf(hdr, sizeof(hdr),
                         "#version 330 core\n"
                         "%s%s%s",
                         m_vb->get_elements(VertexAttribute::Normal)
                             ? "#define PER_VERTEX_NORMALS\n"
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

        auto m = glm::translate(m_model_matrix, m_translation);

        s->setMat4("PVM", cam.m_P * cam.m_V * pt * m);

        if(s->has_uniform("M")) {
            s->setMat4("M", pt * m);
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

        if(scene.m_lightpos) {
            ImGui::SliderFloat3("Ambient", &m_ambient[0], 0, 1);
            ImGui::SliderFloat3("Diffuse", &m_diffuse[0], 0, 1);
            ImGui::SliderFloat3("Specular", &m_specular[0], 0, 1);
        }
        ImGui::SliderFloat("Normals", &m_normal_colorize, 0, 1);

        if(m_elements) {
            ImGui::SliderInt("DrawCount", &m_drawcount, 0, m_elements);
        }
        ImGui::Text("Translation");
        ImGui::SliderFloat("X##t", &m_translation.x, -5000, 5000);
        ImGui::SliderFloat("Y##t", &m_translation.y, -5000, 5000);
        ImGui::SliderFloat("Z##t", &m_translation.z, -5000, 5000);
    }

    void setColor(const glm::vec4 &ambient, const glm::vec4 &diffuse,
                  const glm::vec4 &specular) override
    {
        m_ambient = ambient;
        m_diffuse = diffuse;
        m_specular = specular;
    }

    void update(const std::shared_ptr<Image2D> &img) override
    {
        m_tex0.set(img);
    }

    void set(const std::string &key, float val) override
    {
        if(key == "vertexcolor")
            m_colorize = val;
        if(key == "normalcolors")
            m_normal_colorize = val;
    }

    VertexAttribBuffer m_attrib_buf;
    ArrayBuffer m_index_buf{GL_ELEMENT_ARRAY_BUFFER};

    std::unique_ptr<Shader> m_shader;
    int m_drawcount{0};
    int m_elements{0};

    glm::vec3 m_ambient{1};
    glm::vec3 m_specular{1};
    glm::vec3 m_diffuse{1};

    float m_colorize{1};
    float m_normal_colorize{0};

    bool m_wireframe{false};
    bool m_backface_culling{true};

    Texture2D m_tex0;

    glm::vec3 m_translation{0};

    std::shared_ptr<VertexBuffer> m_vb;
    std::shared_ptr<std::vector<glm::ivec3>> m_ib;
};

std::shared_ptr<Object>
makeMesh(const std::shared_ptr<VertexBuffer> &vb,
         const std::vector<glm::ivec3> &ib, bool interactive)
{
    auto ibsp = std::make_shared<std::vector<glm::ivec3>>(ib);
    return std::make_shared<Mesh>(vb, ibsp);
}

}  // namespace g3d
