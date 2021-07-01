#include "shader.hpp"

#include "object.hpp"


static const char *cb_vertex_shader = R"glsl(
#version 330 core

uniform mat4 PV;
uniform mat4 model;

uniform int widthshift;
uniform float scale;

void main()
{
   int ly = (gl_VertexID & 2) >> 1;
   int lx = (gl_VertexID & 1) ^ ly;

   int gi = gl_VertexID >> 2;
   int gx = 2 * (gi & ((1 << widthshift) - 1)) + (bool((gi & (1 << widthshift))) ? 1 : 0);
   int gy = gi >> widthshift;

   float gt = scale * (1 << widthshift);

   gl_Position = PV * model * vec4(scale * (lx + gx) - gt, scale * (ly + gy) - gt, 0, 1);
}

)glsl";




static const char *cb_fragment_shader = R"glsl(

#version 330 core

out vec4 FragColor;
uniform vec4 col;

void main()
{
  FragColor = col;
}

)glsl";


namespace g3d {

struct CheckerBoard : public Object {

  inline static Shader *s_shader;

  CheckerBoard(float tilesize,
               int num_tiles_per_dimention)
    : m_tilesize(tilesize)
    , m_width_shift(log2f(num_tiles_per_dimention))
  {
    if(!s_shader) {
      s_shader = new Shader(cb_vertex_shader, cb_fragment_shader);
    }
  }


  void draw(const glm::mat4 &PV) override
  {
    s_shader->use();
    s_shader->setMat4("PV", PV);
    s_shader->setInt("widthshift", m_width_shift);
    s_shader->setFloat("scale", m_tilesize);
    s_shader->setMat4("model", m_model_matrix);
    s_shader->setVec4("col", m_color);

    glDrawArrays(GL_QUADS, 0, 8 << (2 * m_width_shift));
  }


  void setColor(const glm::vec4 &c) override
  {
    m_color = c;
  }

  const float m_tilesize;
  const int m_width_shift;

  glm::vec4 m_color{1};
};



std::shared_ptr<Object> makeCheckerBoard(float tilesize,
                                         int num_tiles_per_dimention)
{
  return std::make_shared<CheckerBoard>(tilesize, num_tiles_per_dimention);
}

}
