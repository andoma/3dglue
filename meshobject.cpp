#include "object.hpp"

#include "shader.hpp"
#include "buffer.hpp"
#include "mesh.hpp"
#include "texture.hpp"

extern unsigned char phong_vertex_glsl[];
extern int phong_vertex_glsl_len;
extern unsigned char phong_geometry_glsl[];
extern int phong_geometry_glsl_len;
extern unsigned char phong_fragment_glsl[];
extern int phong_fragment_glsl_len;

namespace g3d {

struct MeshObject : public Object {
    MeshObject(const Mesh &data)
      : m_attrib_buf(&data.m_attributes[0],
                     data.m_attributes.size() * sizeof(float), GL_ARRAY_BUFFER)
      , m_index_buf(&data.m_indicies[0], data.m_indicies.size() * sizeof(float),
                    GL_ELEMENT_ARRAY_BUFFER)
      , m_attr_flags(data.m_attr_flags)
      , m_elements(data.m_indicies.size())
      , m_drawcount(m_elements / 3)
      , m_tex0(data.m_tex0)
      , m_apv(data.m_apv)
      , m_vertices(data.num_vertices())
    {
        char hdr[4096];

        m_name = "Mesh";

        snprintf(
            hdr, sizeof(hdr),
            "#version 330 core\n"
            "%s%s%s",
            has_normals(m_attr_flags) ? "#define PER_VERTEX_NORMALS\n" : "",
            has_per_vertex_color(m_attr_flags) ? "#define PER_VERTEX_COLOR\n"
                                               : "",
            has_uv0(m_attr_flags) ? "#define TEX0\n" : "");

        // clang-format off
        m_shader = std::make_unique<Shader>("phong",
            hdr,
            (const char *)phong_vertex_glsl,   (int)phong_vertex_glsl_len,
            (const char *)phong_fragment_glsl, (int)phong_fragment_glsl_len,
            (const char *)phong_geometry_glsl, (int)phong_geometry_glsl_len);
        // clang-format on
    }

    void draw(const Scene &scene) override
    {
        m_shader->use();

        auto m = glm::translate(m_model_matrix, m_translation);

        m_shader->setMat4("PVM", scene.m_P * scene.m_V * m);

        if(m_shader->has_uniform("M")) {
            m_shader->setMat4("M", m);
        }

        if(has_per_vertex_color(m_attr_flags)) {
            m_shader->setFloat("per_vertex_color_blend", m_colorize);
        }

        if(scene.m_lightpos) {
            m_shader->setVec3("lightPos", *scene.m_lightpos);
            m_shader->setVec3("diffuseColor", m_diffuse);
            m_shader->setVec3("specularColor", m_specular);
            m_shader->setVec3("ambientColor", m_ambient);
        } else {
            m_shader->setVec3("ambientColor", glm::vec3{1.0f});
        }

        m_shader->setFloat("normalColorize", m_normal_colorize);

        if(m_shader->has_uniform("viewPos")) {
            auto V_I = glm::inverse(scene.m_V);
            m_shader->setVec3("viewPos", glm::vec3{V_I[3]});
        }

        glBindBuffer(GL_ARRAY_BUFFER, m_attrib_buf.m_buffer);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, m_apv * sizeof(float),
                              (void *)NULL);

        int off = 3;

        if(has_normals(m_attr_flags)) {
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                                  m_apv * sizeof(float),
                                  (void *)(off * sizeof(float)));
            off += 3;
        }

        if(has_per_vertex_color(m_attr_flags)) {
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE,
                                  m_apv * sizeof(float),
                                  (void *)(off * sizeof(float)));
            off += 4;
        }

        if(has_uv0(m_attr_flags)) {
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE,
                                  m_apv * sizeof(float),
                                  (void *)(off * sizeof(float)));
            off += 2;
        }

        if(m_tex0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_tex0->get());
            m_shader->setInt("tex0", 0);
        }

        if(m_backface_culling)
            glEnable(GL_CULL_FACE);

        if(m_wireframe)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        if(m_elements) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buf.m_buffer);
            glDrawElements(GL_TRIANGLES, m_drawcount * 3, GL_UNSIGNED_INT,
                           NULL);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, m_vertices);
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_CULL_FACE);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
    }

    void ui(const Scene &s) override
    {
        ImGui::Checkbox("Visible", &m_visible);
        ImGui::Checkbox("Wireframe", &m_wireframe);
        ImGui::Checkbox("Backface culling", &m_backface_culling);

        ImGui::SliderFloat("Color", &m_colorize, 0, 1);

        if(s.m_lightpos) {
            ImGui::SliderFloat3("Ambient", &m_ambient[0], 0, 1);
            ImGui::SliderFloat3("Diffuse", &m_diffuse[0], 0, 1);
            ImGui::SliderFloat3("Specular", &m_specular[0], 0, 1);
        }
        ImGui::SliderFloat("Normals", &m_normal_colorize, 0, 1);

        if(m_elements) {
            ImGui::SliderInt("DrawCount", &m_drawcount, 0, m_elements / 3);
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

    ArrayBuffer m_attrib_buf;
    ArrayBuffer m_index_buf;
    const MeshAttributes m_attr_flags;
    const size_t m_elements;
    std::unique_ptr<Shader> m_shader;
    const int m_apv;
    const size_t m_vertices;

    glm::vec3 m_ambient{1};
    glm::vec3 m_specular{1};
    glm::vec3 m_diffuse{1};

    float m_colorize{1};
    float m_normal_colorize{0};

    int m_drawcount{0};
    bool m_wireframe{false};
    bool m_backface_culling{true};

    std::shared_ptr<Texture2D> m_tex0;

    glm::vec3 m_translation{0};
};

std::shared_ptr<Object>
makeMeshObject(const Mesh &data)
{
    return std::make_shared<MeshObject>(data);
}

}  // namespace g3d
