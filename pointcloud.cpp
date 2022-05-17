#include "object.hpp"

#include "buffer.hpp"
#include "shader.hpp"

static const char *pc_vertex_shader = R"glsl(
layout (location = 0) in vec3 aPos;

#ifdef PER_VERTEX_COLOR
layout (location = 1) in vec4 aCol;
#endif

#ifdef PER_VERTEX_TRAIT
layout (location = 2) in float aTrait;
#endif

uniform mat4 PV;
uniform mat4 model;
uniform vec4 albedo;
uniform vec3 bbox1;
uniform vec3 bbox2;
uniform float alpha;
uniform vec2 trait_minmax;

out vec4 fragmentColor;

void main()
{
   gl_Position = PV * model * vec4(aPos.xyz, 1);

   vec3 s = step(bbox1, aPos.xyz) - step(bbox2, aPos.xyz);
   float inside = s.x * s.y * s.z;

   float a = alpha * (inside + 0.25);
#ifdef PER_VERTEX_TRAIT
   a = a * (aTrait >= trait_minmax.x && aTrait <= trait_minmax.y ? 1.0 : 0.0);
#endif

#ifdef PER_VERTEX_COLOR
   fragmentColor = vec4(aCol.xyz, a) * albedo;
#else
   fragmentColor = vec4(1,1,1, a) * albedo;
#endif
}

)glsl";

static const char *pc_fragment_shader = R"glsl(
out vec4 FragColor;
in vec4 fragmentColor;

void main()
{
  if(fragmentColor.a == 0.0) {
    discard;
  } else {
    FragColor = fragmentColor;
  }
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
    {-1, -1, -1}, {1, -1, -1},

    {1, -1, -1},  {1, 1, -1},

    {1, 1, -1},   {-1, 1, -1},

    {-1, 1, -1},  {-1, -1, -1},

    {-1, -1, 1},  {1, -1, 1},

    {1, -1, 1},   {1, 1, 1},

    {1, 1, 1},    {-1, 1, 1},

    {-1, 1, 1},   {-1, -1, 1},

    {-1, -1, -1}, {-1, -1, 1},

    {1, -1, -1},  {1, -1, 1},

    {1, 1, -1},   {1, 1, 1},

    {-1, 1, -1},  {-1, 1, 1},
};

namespace g3d {

struct PointCloud : public Object {
    inline static Shader *s_bb_shader;

    std::unique_ptr<Shader> m_shader;

    ArrayBuffer m_attrib_buf;

    PointCloud(size_t num_points, const float *xyz, const float *rgb,
               const float *trait)
      : m_attrib_buf((void *)&attribs[0][0], sizeof(attribs), GL_ARRAY_BUFFER)
      , m_xyz(xyz, num_points * sizeof(float) * 3, GL_ARRAY_BUFFER)
      , m_rgb(rgb, num_points * sizeof(float) * 3, GL_ARRAY_BUFFER)
      , m_trait(trait, num_points * sizeof(float), GL_ARRAY_BUFFER)
      , m_colorized(!!rgb)
      , m_traited(!!trait)
      , m_num_points(num_points)
    {
        char hdr[4096];

        m_name = "Pointcloud";

        snprintf(hdr, sizeof(hdr),
                 "#version 330 core\n"
                 "%s%s",
                 rgb ? "#define PER_VERTEX_COLOR\n" : "",
                 trait ? "#define PER_VERTEX_TRAIT\n" : "");

        m_shader = std::make_unique<Shader>("pointcloud", hdr, pc_vertex_shader,
                                            -1, pc_fragment_shader, -1);

        if(!s_bb_shader) {
            s_bb_shader = new Shader("pointcloud-bb", NULL, bb_vertex_shader,
                                     -1, bb_fragment_shader, -1);
        }
    }

    void draw(const Scene &scene) override
    {
        Shader *s = m_shader.get();
        s->use();
        s->setMat4("PV", scene.m_P * scene.m_V);
        s->setMat4("model", m_model_matrix);
        s->setVec4("albedo", glm::vec4{glm::vec3{m_color}, 1});
        s->setFloat("alpha", m_alpha);

        if(m_trait_on) {
            s->setVec2("trait_minmax", glm::vec2{m_trait_min, m_trait_max});
        } else {
            s->setVec2("trait_minmax", glm::vec2{-INFINITY, INFINITY});
        }

        if(m_bb) {
            s->setVec3("bbox1", m_bbox_center - m_bbox_size * 0.5f);
            s->setVec3("bbox2", m_bbox_center + m_bbox_size * 0.5f);
        } else {
            s->setVec3("bbox1", glm::vec3{-INFINITY});
            s->setVec3("bbox2", glm::vec3{INFINITY});
        }

        glPointSize(m_pointsize);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, m_xyz.m_buffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, NULL);

        if(m_colorized) {
            glEnableVertexAttribArray(1);
            glBindBuffer(GL_ARRAY_BUFFER, m_rgb.m_buffer);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, 0, NULL);
        }

        if(m_traited) {
            glEnableVertexAttribArray(2);
            glBindBuffer(GL_ARRAY_BUFFER, m_trait.m_buffer);
            glVertexAttribPointer(2, 1, GL_FLOAT, GL_TRUE, 0, NULL);
        }

        glDrawArrays(GL_POINTS, 0, m_num_points);
        glDisableVertexAttribArray(0);
        if(m_colorized) {
            glDisableVertexAttribArray(1);
        }
        if(m_traited) {
            glDisableVertexAttribArray(2);
        }

        if(m_bb) {
            s_bb_shader->use();
            s_bb_shader->setMat4("PV", scene.m_P * scene.m_V);

            auto m = m_model_matrix;

            m = glm::translate(m, m_bbox_center);
            m = glm::scale(m, m_bbox_size * 0.5f);
            s_bb_shader->setMat4("model", m);

            glEnableVertexAttribArray(0);

            glBindBuffer(GL_ARRAY_BUFFER, m_attrib_buf.m_buffer);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, (void *)NULL);

            glDrawArrays(GL_LINES, 0, 24);
            glDisableVertexAttribArray(0);
        }
    }

    void setColor(const glm::vec4 &c) override { m_color = c; }

    void ui(const Scene &s) override
    {
        ImGui::Text("%zd points", m_num_points);
        ImGui::SliderFloat("PointSize", &m_pointsize, 1, 10);

        ImGui::SliderFloat("Alpha", &m_alpha, 0, 1);
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
            m = glm::rotate(m, m_rotation.x, {1, 0, 0});
            m = glm::rotate(m, m_rotation.y, {0, 1, 0});
            m = glm::rotate(m, m_rotation.z, {0, 0, 1});
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

        ImGui::Checkbox("TraitRange", &m_trait_on);
        if(m_trait_on) {
            ImGui::SliderFloat("Min", &m_trait_min, 0, 1);
            ImGui::SliderFloat("Max", &m_trait_max, 0, 1);
        }
    }

    ArrayBuffer m_xyz;
    ArrayBuffer m_rgb;
    ArrayBuffer m_trait;
    const bool m_colorized;
    const bool m_traited;
    const size_t m_num_points;

    glm::vec4 m_color{1};

    glm::vec3 m_translation{0};
    glm::vec3 m_rotation{0};
    glm::vec3 m_bbox_center{0};
    glm::vec3 m_bbox_size{1000};

    bool m_rigid{false};
    bool m_bb{false};
    float m_alpha{1};
    float m_pointsize{1};

    bool m_trait_on{false};
    float m_trait_min{0};
    float m_trait_max{1};
};

std::shared_ptr<Object>
makePointCloud(size_t num_points, const float *xyz, const float *rgb,
               const float *trait)
{
    return std::make_shared<PointCloud>(num_points, xyz, rgb, trait);
}

}  // namespace g3d
