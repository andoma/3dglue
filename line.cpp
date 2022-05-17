#include "object.hpp"

#include "shader.hpp"
#include "buffer.hpp"

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

struct Line : public Object {
    inline static Shader *s_shader;

    ArrayBuffer m_attrib_buf;
    Line(const glm::vec3 segment[2])
      : m_attrib_buf((void *)&segment[0][0], sizeof(glm::vec3) * 2,
                     GL_ARRAY_BUFFER)
    {
        if(!s_shader) {
            s_shader = new Shader("line", NULL, line_vertex_shader, -1,
                                  line_fragment_shader, -1);
        }
    }

    void draw(const Scene &s) override
    {
        s_shader->use();
        s_shader->setMat4("PV", s.m_P * s.m_V);
        s_shader->setMat4("model", m_model_matrix);
        s_shader->setVec4("col", m_color);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, m_attrib_buf.m_buffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, (void *)NULL);
        glDrawArrays(GL_LINES, 0, 2);
        glDisableVertexAttribArray(0);
    }

    void setColor(const glm::vec4 &ambient, const glm::vec4 &diffuse,
                  const glm::vec4 &specular) override
    {
        m_color = ambient;
    }

    glm::vec4 m_color{1};
};

std::shared_ptr<Object>
makeLine(const glm::vec3 segment[2])
{
    return std::make_shared<Line>(segment);
}

}  // namespace g3d
