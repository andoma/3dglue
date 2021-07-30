#include "object.hpp"


#include "shader.hpp"
#include "buffer.hpp"
#include "meshdata.hpp"

// =================================================================

static const char *mesh_vertex_shader = R"glsl(

layout (location = 0) in vec3 aPos;

#ifdef LIGHTING
layout (location = 1) in vec3 aNor;
#endif

#ifdef PER_VERTEX_COLOR
layout (location = 2) in vec4 aCol;
#endif

uniform mat4 PVM;

#ifdef LIGHTING
uniform mat4 M;
out vec3 vNormal;
out vec3 vFragPos;
#endif

uniform vec3 objectColor;
out vec3 vColor;

void main()
{
   gl_Position = PVM * vec4(aPos.xyz, 1);
#ifdef LIGHTING
   vFragPos = (M * vec4(aPos.xyz, 1)).xyz;
   vNormal = aNor;
#endif

#ifdef PER_VERTEX_COLOR
   vColor = objectColor * aCol.xyz;
#else
   vColor = objectColor;
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
#endif


void main()
{
#ifdef LIGHTING
  vec3 lightDir = normalize(lightPos - vFragPos);
  vec3 diffuse  = max(dot(vNormal, lightDir), 0.0) * lightColor;
  vec3 result = (ambient + diffuse) * vColor;
#else
  vec3 result = vColor;
#endif
  FragColor = vec4(result, 1.0);
}

)glsl";







namespace g3d {

struct Mesh : public Object {

  Mesh(const MeshData &data)
    : m_attrib_buf(&data.m_attributes[0],
                   data.m_attributes.size() * sizeof(float),
                   GL_ARRAY_BUFFER)
    , m_index_buf(&data.m_indicies[0],
                  data.m_indicies.size() * sizeof(float),
                  GL_ELEMENT_ARRAY_BUFFER)
    , m_normals(data.m_normals)
    , m_per_vertex_color(data.m_per_vertex_color)
    , m_elements(data.m_indicies.size())
  {
    char hdr[4096];

    snprintf(hdr, sizeof(hdr),
             "#version 330 core\n"
             "%s\n"
             "%s\n",
             m_normals ? "#define LIGHTING" : "",
             m_per_vertex_color ? "#define PER_VERTEX_COLOR" : "");

    std::string vertex_shader(hdr);
    std::string fragment_shader(hdr);

    vertex_shader += mesh_vertex_shader;
    fragment_shader += mesh_fragment_shader;

    m_shader = new Shader(vertex_shader.c_str(),
                          fragment_shader.c_str());

    int apv = 3;

    if(m_normals)
      apv += 3;
    if(m_per_vertex_color)
      apv += 4;

    m_vertices = data.m_attributes.size() / apv;
    m_apv = apv;
  }

  void draw(const glm::mat4 &P, const glm::mat4 &V) override
  {
    if(!m_visible)
      return;

    m_shader->use();

    m_shader->setVec3("objectColor", {1, 1, 1});
    m_shader->setMat4("PVM", P * V * m_model_matrix);

    if(m_shader->has_uniform("M"))
      m_shader->setMat4("M", m_model_matrix);

    if(m_shader->has_uniform("lightPos")) {
      m_shader->setVec3("lightPos", {-2000,-2000,2000});
      m_shader->setVec3("lightColor", {0.7,0.7,0.7});
      m_shader->setVec3("ambient", {0.25, 0.25, 0.25});
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_attrib_buf.m_buffer);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, m_apv * sizeof(float),
                          (void*)NULL);

    int off = 3;

    if(m_normals) {
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, m_apv * sizeof(float),
                            (void*)(off * sizeof(float)));
      off += 3;
    }

    if(m_per_vertex_color) {
      glEnableVertexAttribArray(2);
      glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, m_apv * sizeof(float),
                            (void*)(off * sizeof(float)));
      off += 4;
    }

    if(m_wireframe)
      glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);

    if(m_elements) {
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buf.m_buffer);
      glDrawElements(GL_TRIANGLES, m_elements, GL_UNSIGNED_INT, NULL);
    } else {
      glDrawArrays(GL_TRIANGLES, 0, m_vertices);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
  }

  void prepare() override
  {
    if(ImGui::Begin(m_name.size() ? m_name.c_str() : "Mesh")) {
      ImGui::Checkbox("Visible", &m_visible);
      ImGui::Checkbox("Wireframe", &m_wireframe);
    }
    ImGui::End();
  }


  ArrayBuffer m_attrib_buf;
  ArrayBuffer m_index_buf;
  const bool m_normals;
  const bool m_per_vertex_color;
  const size_t m_elements;
  Shader *m_shader;
  int m_apv;
  size_t m_vertices;

  bool m_visible{true};
  bool m_wireframe{false};
};



std::shared_ptr<Object> makeMesh(const MeshData &data)
{
  return std::make_shared<Mesh>(data);
}

}
