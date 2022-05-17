#include "object.hpp"

#include "shader.hpp"
#include "buffer.hpp"
#include "mesh.hpp"
#include "texture.hpp"

// =================================================================

static const char *mesh_vertex_shader = R"glsl(

layout (location = 0) in vec3 aPos;

#ifdef LIGHTING
layout (location = 1) in vec3 aNor;
#endif

#ifdef PER_VERTEX_COLOR
layout (location = 2) in vec4 aCol;
#endif

#ifdef TEX0
layout (location = 3) in vec2 uv0;
#endif

uniform mat4 PVM;

#ifdef LIGHTING
uniform mat4 M;
out vec3 vNormal;
out vec3 vFragPos;
#endif

uniform float colorize;

out vec3 vColor;

#ifdef TEX0
out vec2 vUV0;
#endif

void main()
{
   gl_Position = PVM * vec4(aPos.xyz, 1);
#ifdef LIGHTING
   vFragPos = (M * vec4(aPos.xyz, 1)).xyz;
   vNormal = aNor;
#endif

#ifdef PER_VERTEX_COLOR
   vColor = mix(vec3(1,1,1), aCol.xyz, colorize);
#else
   vColor = vec3(1,1,1);
#endif

#ifdef TEX0
  vUV0 = uv0;
#endif

}

)glsl";

// =================================================================

static const char *mesh_fragment_shader = R"glsl(

out vec4 FragColor;
in vec3 vColor;

#ifdef LIGHTING
in vec3 vFragPos;
in vec3 vNormal;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 ambient;
uniform float normalColorize;
#endif

#ifdef TEX0
uniform sampler2D tex0;
in vec2 vUV0;
#endif

void main()
{
  vec3 col = vColor;

#ifdef TEX0
  col = col * texture(tex0, vUV0).rgb;
#endif

#ifdef LIGHTING
  vec3 lightDir = normalize(lightPos - vFragPos);
  vec3 diffuse  = max(dot(vNormal, lightDir), 0.0) * lightColor;
  vec3 result = (ambient + diffuse) * col;

  vec3 ncol = vec3(0.5, 0.5, 0.5) * vNormal + vec3(0.5, 0.5, 0.5);
  result = mix(result, ncol, normalColorize);
#else
  vec3 result = col;
#endif
  FragColor = vec4(result, 1.0);
}

)glsl";

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
    {
        char hdr[4096];

        m_name = "Mesh";

        snprintf(hdr, sizeof(hdr),
                 "#version 330 core\n"
                 "%s%s%s",
                 has_normals(m_attr_flags) ? "#define LIGHTING\n" : "",
                 has_per_vertex_color(m_attr_flags)
                     ? "#define PER_VERTEX_COLOR\n"
                     : "",
                 has_uv0(m_attr_flags) ? "#define TEX0\n" : "");

        std::string vertex_shader(hdr);
        std::string fragment_shader(hdr);

        vertex_shader += mesh_vertex_shader;
        fragment_shader += mesh_fragment_shader;

        m_shader = std::make_unique<Shader>(vertex_shader.c_str(),
                                            fragment_shader.c_str());

        int apv = 3;

        if(has_normals(m_attr_flags))
            apv += 3;
        if(has_per_vertex_color(m_attr_flags))
            apv += 4;
        if(has_uv0(m_attr_flags))
            apv += 2;

        m_vertices = data.m_attributes.size() / apv;
        m_apv = apv;
    }

    void draw(const glm::mat4 &P, const glm::mat4 &V) override
    {
        m_shader->use();

        auto m = glm::translate(m_model_matrix, m_translation);

        m_shader->setMat4("PVM", P * V * m);

        if(m_shader->has_uniform("M")) {
            m_shader->setMat4("M", m);
        }

        if(has_per_vertex_color(m_attr_flags)) {
            m_shader->setFloat("colorize", m_colorize);
        }

        if(m_shader->has_uniform("lightPos")) {
            m_shader->setVec3("lightPos", {-2000, -2000, 2000});
            m_shader->setVec3("lightColor", glm::vec3{m_lighting});
            m_shader->setVec3("ambient", glm::vec3{1.0f - m_lighting});
            m_shader->setFloat("normalColorize", m_normal_colorize);
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

    void ui() override
    {
        ImGui::Checkbox("Visible", &m_visible);
        ImGui::Checkbox("Wireframe", &m_wireframe);
        ImGui::Checkbox("Backface culling", &m_backface_culling);

        ImGui::SliderFloat("Color", &m_colorize, 0, 1);

        if(has_normals(m_attr_flags)) {
            ImGui::SliderFloat("Lighting", &m_lighting, 0, 1);
            ImGui::SliderFloat("Normals", &m_normal_colorize, 0, 1);
        }
        if(m_elements) {
            ImGui::SliderInt("DrawCount", &m_drawcount, 0, m_elements / 3);
        }
        ImGui::Text("Translation");
        ImGui::SliderFloat("X##t", &m_translation.x, -5000, 5000);
        ImGui::SliderFloat("Y##t", &m_translation.y, -5000, 5000);
        ImGui::SliderFloat("Z##t", &m_translation.z, -5000, 5000);
    }

    ArrayBuffer m_attrib_buf;
    ArrayBuffer m_index_buf;
    const MeshAttributes m_attr_flags;
    const size_t m_elements;
    std::unique_ptr<Shader> m_shader;
    int m_apv;
    size_t m_vertices;

    float m_lighting{0.75};
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
