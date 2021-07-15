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
uniform vec3 bbox1;
uniform vec3 bbox2;

out vec4 fragmentColor;

void main()
{
   gl_Position = PV * model * vec4(pos.xyz, 1);

   vec3 s = step(bbox1, pos.xyz) - step(bbox2, pos.xyz);
   float inside = s.x * s.y * s.z;

   fragmentColor = vec4(1,1,1, inside + 0.25) * albedo;
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








static const char *bb_vertex_shader = R"glsl(
#version 330 core
layout (location = 0) in vec3 pos;

uniform mat4 PV;
uniform mat4 model;
out vec4 fragmentColor;

void main()
{
   gl_Position = PV * model * vec4(pos.xyz, 1);
}

)glsl";


static const char *bb_fragment_shader = R"glsl(

#version 330 core
out vec4 FragColor;

void main()
{
  FragColor = vec4(1,1,1,1);
}

)glsl";


static float attribs[][3] = {
  {-1,  -1,  -1},
  { 1,  -1,  -1},

  { 1,  -1,  -1},
  { 1,   1,  -1},

  { 1,   1,  -1},
  {-1,   1,  -1},

  {-1,   1,  -1},
  {-1,  -1,  -1},


  {-1,  -1,   1},
  { 1,  -1,   1},

  { 1,  -1,   1},
  { 1,   1,   1},

  { 1,   1,   1},
  {-1,   1,   1},

  {-1,   1,   1},
  {-1,  -1,   1},


  {-1,  -1,  -1},
  {-1,  -1,   1},

  { 1,  -1,  -1},
  { 1,  -1,   1},

  { 1,   1,  -1},
  { 1,   1,   1},

  {-1,   1,  -1},
  {-1,   1,   1},
};




namespace g3d {

struct PointCloud : public Object {

  inline static Shader *s_pc_shader;
  inline static Shader *s_bb_shader;

  ArrayBuffer m_attrib_buf;

  PointCloud(size_t num_points,
             const float *xyz,
             const float *rgb)
    : m_attrib_buf((void *)&attribs[0][0], sizeof(attribs))
    , m_xyz(xyz, num_points * sizeof(float) * 3)
    , m_rgb(rgb, num_points * sizeof(float) * 3)
    , m_colorized(!!rgb)
    , m_num_points(num_points)
  {
    if(!s_pc_shader) {
      s_pc_shader = new Shader(pc_vertex_shader, pc_fragment_shader);
    }

    if(!s_bb_shader) {
      s_bb_shader = new Shader(bb_vertex_shader, bb_fragment_shader);
    }
  }

  void draw(const glm::mat4 &P, const glm::mat4 &V) override
  {
    if(!m_visible)
      return;

    s_pc_shader->use();
    s_pc_shader->setMat4("PV", P * V);
    s_pc_shader->setMat4("model", m_model_matrix);
    s_pc_shader->setVec4("albedo", m_color);

    if(m_bb) {
      s_pc_shader->setVec3("bbox1", m_bbox_center - m_bbox_size * 0.5f);
      s_pc_shader->setVec3("bbox2", m_bbox_center + m_bbox_size * 0.5f);
    } else {
      s_pc_shader->setVec3("bbox1", {NAN, NAN, NAN});
      s_pc_shader->setVec3("bbox2", {NAN, NAN, NAN});
    }

    glPointSize(1); // TODO: Make configurable

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

    if(m_bb) {
      s_bb_shader->use();
      s_bb_shader->setMat4("PV", P * V);

      auto m = m_model_matrix;

      m = glm::translate(m, m_bbox_center);
      m = glm::scale(m, m_bbox_size * 0.5f);
      s_bb_shader->setMat4("model", m);

      glEnableVertexAttribArray(0);

      glBindBuffer(GL_ARRAY_BUFFER, m_attrib_buf.m_buffer);

      glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, (void*)NULL);

      glDrawArrays(GL_LINES, 0, 24);
      glDisableVertexAttribArray(0);
    }
  }

  void setColor(const glm::vec4 &c) override
  {
    m_color = c;
  }

  void prepare() override
  {
    if(ImGui::Begin(m_name.size() ? m_name.c_str() : "Pointcloud")) {

      ImGui::Text("%d points", m_num_points);

      ImGui::Checkbox("Visible", &m_visible);

      ImGui::Checkbox("Rigid Transform", &m_rigid);


      if(m_rigid) {
        ImGui::Text("Translation");
        ImGui::SliderFloat("X##t", &m_translation.x, -5000, 5000);
        ImGui::SliderFloat("Y##t", &m_translation.y, -5000, 5000);
        ImGui::SliderFloat("Z##t", &m_translation.z, -5000, 5000);

        ImGui::Text("Rotation");
        ImGui::SliderAngle("X##r", &m_rotation.x);
        ImGui::SliderAngle("Y##r", &m_rotation.y);
        ImGui::SliderAngle("Z##r", &m_rotation.z);

        auto m = glm::mat4(1);
        m = glm::translate(m, m_translation);
        m = glm::rotate(m, m_rotation.x, {1,0,0});
        m = glm::rotate(m, m_rotation.y, {0,1,0});
        m = glm::rotate(m, m_rotation.z, {0,0,1});
        m_model_matrix = m;
      }

      ImGui::Checkbox("BoundingBox", &m_bb);
      if(m_bb) {
        ImGui::Text("Position");
        ImGui::SliderFloat("X##c", &m_bbox_center.x, -5000, 5000);
        ImGui::SliderFloat("Y##c", &m_bbox_center.y, -5000, 5000);
        ImGui::SliderFloat("Z##c", &m_bbox_center.z, -5000, 5000);
        ImGui::Text("Size");
        ImGui::SliderFloat("X##s", &m_bbox_size.x, 1, 5000);
        ImGui::SliderFloat("Y##s", &m_bbox_size.y, 1, 5000);
        ImGui::SliderFloat("Z##s", &m_bbox_size.z, 1, 5000);
      }

    }
    ImGui::End();
  }

  ArrayBuffer m_xyz;
  ArrayBuffer m_rgb;
  const bool m_colorized;
  const size_t m_num_points;

  glm::vec4 m_color{1};

  glm::vec3 m_translation{0};
  glm::vec3 m_rotation{0};

  glm::vec3 m_bbox_center{0};
  glm::vec3 m_bbox_size{1000};

  bool m_visible{true};
  bool m_rigid{false};
  bool m_bb{false};
};


std::shared_ptr<Object> makePointCloud(size_t num_points,
                                       const float *xyz,
                                       const float *rgb)
{
  return std::make_shared<PointCloud>(num_points, xyz, rgb);
}


}
