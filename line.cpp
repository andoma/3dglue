#include "object.hpp"

#include "shader.hpp"
#include "buffer.hpp"
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

struct LineStrip : public Object {
    inline static Shader *s_shader;

    ArrayBuffer m_attrib_buf;
    LineStrip(const std::vector<glm::vec3> strip, GLenum mode)
      : m_attrib_buf((void *)&strip[0][0], sizeof(glm::vec3) * strip.size(),
                     GL_ARRAY_BUFFER)
      , m_count(strip.size())
      , m_draw_count(m_count)
      , m_mode(mode)
    {
        m_name = "Lines";
        if(!s_shader) {
            s_shader = new Shader("line", NULL, line_vertex_shader, -1,
                                  line_fragment_shader, -1);
        }
    }

    void draw(const Scene &scene, const Camera &cam,
              const glm::mat4 &pt) override
    {
        s_shader->use();
        s_shader->setMat4("PV", cam.m_P * cam.m_V);
        s_shader->setMat4("model", pt * m_model_matrix);
        s_shader->setVec4("col", m_color);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, m_attrib_buf.m_buffer);
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
        ImGui::SliderInt("DrawCount", &m_draw_count, 0, m_count);
    }

    void update(const float *attributes, size_t count) override
    {
        m_attrib_buf.write(attributes, sizeof(float) * count);
        m_count = count / 3;
        m_draw_count = m_count;
    }

    glm::vec4 m_color{1};
    size_t m_count;
    int m_draw_count;
    const GLenum m_mode;
};

std::shared_ptr<Object>
makeLine(const glm::vec3 segment[2])
{
    std::vector<glm::vec3> lines{segment[0], segment[1]};
    return std::make_shared<LineStrip>(lines, GL_LINES);
}

std::shared_ptr<Object>
makeLineStrip(const std::vector<glm::vec3> &linestrip)
{
    return std::make_shared<LineStrip>(linestrip, GL_LINE_STRIP);
}

std::shared_ptr<Object> makeLines(const std::vector<glm::vec3> &lines)
{
    return std::make_shared<LineStrip>(lines, GL_LINES);
}

}  // namespace g3d
