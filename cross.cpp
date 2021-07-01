#include "object.hpp"

#include "shader.hpp"
#include "buffer.hpp"


// from https://gist.github.com/rikusalminen/9393151
static const char *cross_vertex_shader = R"glsl(
#version 330 core
layout (location = 0) in vec3 vtx;
layout (location = 1) in vec4 col;

uniform mat4 PV;
uniform mat4 model;

out vec4 fragmentColor;

void main()
{
   gl_Position = PV * model * vec4(vtx.xyz, 1);

   fragmentColor = col;
}

)glsl";



static const char *cross_fragment_shader = R"glsl(

#version 330 core

out vec4 FragColor;
in vec4 fragmentColor;

void main()
{
  FragColor = fragmentColor;
}

)glsl";


static float attribs[6][7] = {
  {-1,  0,  0,    1, 0, 0, 0.2},
  { 1,  0,  0,    1, 0, 0, 1},
  { 0, -1,  0,    0, 1, 0, 0.2},
  { 0,  1,  0,    0, 1, 0, 1},
  { 0,  0, -1,    0, 0, 1, 0.2},
  { 0,  0,  1,    0, 0, 1, 1},
};

namespace g3d {

struct Cross : public Object {

  inline static Shader *s_shader;

  ArrayBuffer m_attrib_buf;
  Cross()
    : m_attrib_buf((void *)&attribs[0][0], sizeof(attribs))
  {
    if(!s_shader) {
      s_shader = new Shader(cross_vertex_shader, cross_fragment_shader);
    }
  }


  void draw(const glm::mat4 &PV) override
  {
    s_shader->use();
    s_shader->setMat4("PV", PV);
    s_shader->setMat4("model", m_model_matrix);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, m_attrib_buf.m_buffer);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 7 * sizeof(float),
                          (void*)NULL);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_TRUE, 7 * sizeof(float),
                          (void*)(3 * sizeof(float)));

    glDrawArrays(GL_LINES, 0, 6);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
  }
};



std::shared_ptr<Object> makeCross()
{
  return std::make_shared<Cross>();
}

}
