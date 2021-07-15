#include "shader.hpp"

#include "object.hpp"


// from https://gist.github.com/rikusalminen/9393151
static const char *cube_vertex_shader = R"glsl(
#version 330 core

uniform mat4 PV;
uniform mat4 model;
out vec4 col;

void main()
{
    int tri = gl_VertexID / 3;
    int idx = gl_VertexID % 3;
    int face = tri / 2;
    int top = tri % 2;

    int dir = face % 3;
    int pos = face / 3;

    int nz = dir >> 1;
    int ny = dir & 1;
    int nx = 1 ^ (ny | nz);

    vec3 d = vec3(nx, ny, nz);
    float flip = 1 - 2 * pos;

    vec3 n = flip * d;
    vec3 u = -d.yzx;
    vec3 v = flip * d.zxy;

    float mirror = -1 + 2 * top;
    vec3 xyz = n + mirror*(1-2*(idx&1))*u + mirror*(1-2*(idx>>1))*v;

    gl_Position = PV * model * vec4(xyz, 1.0);
    col = vec4(0.5, 0.5, 0.5, 1.0) + vec4(xyz * 0.3, 0.0);
}

)glsl";



static const char *cube_fragment_shader = R"glsl(

#version 330 core

in vec4 col;
out vec4 FragColor;

void main()
{
  FragColor = col;
}

)glsl";


namespace g3d {

struct Cube : public Object {

  inline static Shader *s_shader;

  Cube()
  {
    if(!s_shader) {
      s_shader = new Shader(cube_vertex_shader, cube_fragment_shader);
    }
  }


  void draw(const glm::mat4 &P, const glm::mat4 &V) override
  {
    s_shader->use();
    s_shader->setMat4("PV", P * V);
    s_shader->setMat4("model", m_model_matrix);
    glDrawArrays(GL_TRIANGLES, 0, 36);
  }
};



std::shared_ptr<Object> makeCube()
{
  return std::make_shared<Cube>();
}

}
