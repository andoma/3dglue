#include "object.hpp"

#include "shader.hpp"
#include "buffer.hpp"

static const char *skybox_vertex_shader = R"glsl(
#version 330 core
layout (location = 0) in vec2 vtx;
out vec2 coord;

void main()
{
   gl_Position = vec4(vtx.xy, 0, 1);
   coord = vtx;
}

)glsl";


static const char *skybox_fragment_shader = R"glsl(

#version 330 core

uniform mat4 PVinv;

out vec4 FragColor;
in vec2 coord;
uniform vec3 cam;
uniform float scale;

float checkerboard(vec2 p,float size){
  p *= size;
  vec2 f=fract(p.xy)-0.5;
  return f.x * f.y > 0.0 ? 1.0 : 0.0;
}

void main()
{
  vec4 p = PVinv * vec4(coord, 0, 1);
  vec3 dir = normalize(p.xyz);

  if(dir.z > 0) {

    FragColor = mix(vec4(0.5, 0.5, 1, 1), vec4(0, 0, 0.5, 1), dir.z);

  } else {

    vec3 ground = vec3(0,0,-1);
    float t = -(dot(ground, cam) + 0) / dot(ground, dir);

    if(t <= 0) {
      FragColor = vec4(0, 0, 0, 1);
    } else {
      vec3 floorpoint = cam + t * dir;

      float fog = max(1 - (t * scale * 0.05), 0);

      vec3 col = mix(vec3(0,0.6,0), vec3(0,1,0), checkerboard(floorpoint.xy, scale));
      col = mix(vec3(0,0.8,0), col, fog);

      FragColor = vec4(col, 1);
    }
  }
}

)glsl";



static const float attribs[6][2] = {
  {-1, -1},
  { 1, -1},
  { 1,  1},

  {-1, -1},
  { 1,  1},
  {-1,  1},
};

namespace g3d {

struct Skybox : public Object {

  inline static Shader *s_shader;

  ArrayBuffer m_attrib_buf;

  Skybox(float checkersize)
    : m_attrib_buf((void *)&attribs[0][0], sizeof(attribs),
                   GL_ARRAY_BUFFER)
    , m_checkersize(checkersize)
  {
    if(!s_shader) {
      s_shader = new Shader(skybox_vertex_shader, skybox_fragment_shader);
    }
  }


  void draw(const glm::mat4 &P, const glm::mat4 &V) override
  {
    s_shader->use();
    glBindBuffer(GL_ARRAY_BUFFER, m_attrib_buf.m_buffer);

    s_shader->setVec3("cam", glm::inverse(V)[3]);
    s_shader->setMat4("PVinv", glm::inverse(P * V));
    s_shader->setFloat("scale", 0.5f / m_checkersize);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, 0, NULL);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(0);
  }


  void prepare() override
  {
    if(ImGui::Begin(m_name.size() ? m_name.c_str() : "Skybox")) {
      ImGui::Checkbox("Visible", &m_visible);
      ImGui::SliderFloat("CheckerSize", &m_checkersize, 10, 10000,
                         "%.1f", ImGuiSliderFlags_Logarithmic);

    }
    ImGui::End();
  }

  float m_checkersize{1000};
};


std::shared_ptr<Object> makeSkybox(float checkersize)
{
  return std::make_shared<Skybox>(checkersize);
}

}
