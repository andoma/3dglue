#include "object.hpp"

#include "shader.hpp"
#include "buffer.hpp"

#include <glm/gtx/string_cast.hpp>

static const char *mesh_vertex_shader_rgba = R"glsl(
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 2) in vec4 aCol;

uniform mat4 PVM;

out vec4 fragmentColor;

void main()
{
   gl_Position = PVM * vec4(aPos.xyz, 1);
   fragmentColor = aCol;
}

)glsl";


static const char *mesh_fragment_shader_rgba = R"glsl(

#version 330 core

out vec4 FragColor;
in vec4 fragmentColor;

void main()
{
  FragColor = fragmentColor;
}

)glsl";


// =================================================================

static const char *mesh_vertex_shader_lighting = R"glsl(
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNor;

uniform mat4 PVM;
uniform mat4 M;

out vec3 FragPos;
out vec3 Normal;

void main()
{
   gl_Position = PVM * vec4(aPos.xyz, 1);
   FragPos = (M * vec4(aPos.xyz, 1)).xyz;
   Normal = aNor;
}

)glsl";


static const char *mesh_fragment_shader_lighting = R"glsl(

#version 330 core

out vec4 FragColor;
in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 ambient;
uniform vec3 objectColor;

void main()
{
  vec3 lightDir = normalize(lightPos - FragPos);
  vec3 diffuse  = max(dot(Normal, lightDir), 0.0) * lightColor;
  vec3 result = (ambient + diffuse) * objectColor;
  FragColor = vec4(result, 1.0);
}

)glsl";



namespace g3d {

struct Mesh : public Object {


  Mesh(const std::vector<float> &attributes, int flags)
    : m_attrib_buf(&attributes[0], attributes.size() * sizeof(float))
    , m_flags(flags)
  {
    if(flags & MESH_NORMALS) {
      m_shader = new Shader(mesh_vertex_shader_lighting,
                            mesh_fragment_shader_lighting);
    } else {
      m_shader = new Shader(mesh_vertex_shader_rgba,
                            mesh_fragment_shader_rgba);
    }

    int apv = 3;

    if(flags & MESH_NORMALS)
      apv += 3;
    if(flags & MESH_PER_VERTEX_COLOR)
      apv += 4;

    m_vertices = attributes.size() / apv;
    m_apv = apv;
  }

  void draw(const glm::mat4 &P, const glm::mat4 &V) override
  {
    m_shader->use();
    if(m_shader->has_uniform("PVM"))
      m_shader->setMat4("PVM", P * V * m_model_matrix);

    if(m_shader->has_uniform("M"))
      m_shader->setMat4("M", m_model_matrix);

    if(m_shader->has_uniform("lightPos")) {
      m_shader->setVec3("lightPos", {-2000,-2000,2000});
      m_shader->setVec3("lightColor", {0.7,0.7,0.7});
      m_shader->setVec3("ambient", {0.25, 0.25, 0.25});
      m_shader->setVec3("objectColor", {1, 1, 1});
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_attrib_buf.m_buffer);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, m_apv * sizeof(float),
                          (void*)NULL);

    int off = 3;

    if(m_flags & MESH_NORMALS) {
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, m_apv * sizeof(float),
                            (void*)(off * sizeof(float)));
      off += 3;
    }

    if(m_flags & MESH_PER_VERTEX_COLOR) {
      glEnableVertexAttribArray(2);
      glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, m_apv * sizeof(float),
                            (void*)(off * sizeof(float)));
      off += 4;
    }

    glDrawArrays(GL_TRIANGLES, 0, m_vertices);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
  }

  Shader *m_shader;
  ArrayBuffer m_attrib_buf;
  const int m_flags;
  int m_apv;
  size_t m_vertices;
 };



std::shared_ptr<Object> makeMesh(const std::vector<float> &attributes,
                                 int flags)
{
  return std::make_shared<Mesh>(attributes, flags);
}

}
