#include "object.hpp"

#include "shader.hpp"
#include "arraybuffer.hpp"
#include "camera.hpp"

static const char *line_vertex_shader = R"glsl(
#version 330 core
layout (location = 0) in vec3 vtx;

uniform mat4 PV;
uniform mat4 model;
uniform vec4 col;

out vec4 fragmentColor;

void main()
{
   gl_Position = PV * model * vec4(vtx.xyz, 1);
   fragmentColor = col;
}

)glsl";

static const char *line_fragment_shader = R"glsl(

#version 330 core

out vec4 FragColor;
in vec4 fragmentColor;

void main()
{
  FragColor = fragmentColor;
}

)glsl";

namespace g3d {

struct Lines : public Object {
    inline static Shader *s_shader;

    Lines(GLenum mode, const std::shared_ptr<VertexBuffer> &vb)
      : m_mode(mode), m_vb(vb)
    {
        m_name = "Lines";
    }

    void draw(const Scene &scene, const Camera &cam,
              const glm::mat4 &pt) override
    {
        if(!s_shader) {
            s_shader = new Shader("line", NULL, line_vertex_shader, -1,
                                  line_fragment_shader, -1);
        }

        s_shader->use();
        s_shader->setMat4("PV", cam.m_P * cam.m_V);
        s_shader->setMat4("model", pt * m_model_matrix);
        s_shader->setVec4("col", m_color);

        if(m_vb) {
            m_attrib_buf.load(*m_vb);
            m_vb.reset();
            m_draw_count = m_attrib_buf.size();
        }

        if(!m_attrib_buf.bind())
            return;

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, (void *)NULL);
        glDrawArrays(m_mode, 0, m_draw_count);
        glDisableVertexAttribArray(0);
    }

    void setColor(const glm::vec4 &ambient, const glm::vec4 &diffuse,
                  const glm::vec4 &specular) override
    {
        m_color = ambient;
    }

    void ui(const Scene &scene) override
    {
        ImGui::Checkbox("Visible", &m_visible);
        ImGui::SliderInt("DrawCount", &m_draw_count, 0, m_attrib_buf.size());
    }

    void set(const std::shared_ptr<VertexBuffer> &vb) override { m_vb = vb; }

    const GLenum m_mode;
    std::shared_ptr<VertexBuffer> m_vb;
    VertexAttribBuffer m_attrib_buf;

    glm::vec4 m_color{1};
    int m_draw_count{0};
};

std::shared_ptr<Object>
makeLines(const std::vector<glm::vec3> &lines)
{
    return std::make_shared<Lines>(GL_LINES, VertexBuffer::make(lines));
}

std::shared_ptr<Object>
makeLine(const glm::vec3 segment[2])
{
    std::vector<glm::vec3> lines{segment[0], segment[1]};
    return makeLines(lines);
}

std::shared_ptr<Object>
makeLineStrip(const std::vector<glm::vec3> &linestrip)
{
    return std::make_shared<Lines>(GL_LINE_STRIP,
                                   VertexBuffer::make(linestrip));
}

}  // namespace g3d
