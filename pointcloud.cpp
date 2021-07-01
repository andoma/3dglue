#include "object.hpp"

#include "shader.hpp"
#include "buffer.hpp"


static const char *pc_vertex_shader = R"glsl(
#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 col;

uniform mat4 PV;
uniform mat4 model;
uniform vec4 albedo;

out vec4 fragmentColor;

void main()
{
   gl_Position = PV * model * vec4(pos.xyz, 1);
   fragmentColor = vec4(col.rgb, 1) * albedo;
}

)glsl";


static const char *pc_fragment_shader = R"glsl(

#version 330 core

out vec4 FragColor;
in vec4 fragmentColor;

void main()
{
  FragColor = fragmentColor;
}

)glsl";


namespace g3d {

struct PointCloud : public Object {

  inline static Shader *s_shader;

  PointCloud(size_t num_points,
             const float *xyz,
             const float *rgb)
    : m_xyz(xyz, num_points * sizeof(float) * 3)
    , m_rgb(rgb, num_points * sizeof(float) * 3)
    , m_colorized(!!rgb)
    , m_num_points(num_points)
  {
    if(!s_shader) {
      s_shader = new Shader(pc_vertex_shader, pc_fragment_shader);
    }
  }

  void draw(const glm::mat4 &PV) override
  {
    s_shader->use();
    s_shader->setMat4("PV", PV);
    s_shader->setMat4("model", m_model_matrix);
    s_shader->setVec4("albedo", m_color);

    glPointSize(2); // TODO: Make configurable

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, m_xyz.m_buffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, NULL);

    if(m_colorized) {
      glEnableVertexAttribArray(1);
      glBindBuffer(GL_ARRAY_BUFFER, m_rgb.m_buffer);
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, 0, NULL);
    }

    glDrawArrays(GL_POINTS, 0, m_num_points);
    glDisableVertexAttribArray(0);
    if(m_colorized) {
      glDisableVertexAttribArray(1);
    }

  }

  void setColor(const glm::vec4 &c) override
  {
    m_color = c;
  }


  ArrayBuffer m_xyz;
  ArrayBuffer m_rgb;
  const bool m_colorized;
  const size_t m_num_points;
  glm::vec4 m_color{1};
};


std::shared_ptr<Object> makePointCloud(size_t num_points,
                                       const float *xyz,
                                       const float *rgb)
{
  return std::make_shared<PointCloud>(num_points, xyz, rgb);
}


}
