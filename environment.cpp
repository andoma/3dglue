#include "object.hpp"

#include "shader.hpp"
#include "buffer.hpp"

static const char *viewport_vertex_shader = R"glsl(
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

  vec3 bg;

  if(dir.z > 0) {
    // Sky
    bg = mix(vec3(0.5, 0.5, 1), vec3(0, 0, 0.5), dir.z);
  } else {
    // Ground
    bg = mix(vec3(0.0, 0.2, 0.0), vec3(0, 0, 0), -dir.z);
  }
  FragColor = vec4(bg, 1);
}

)glsl";

__attribute__((unused)) static const char *ground_fragment_shader = R"glsl(

#version 330 core

uniform mat4 PVinv;
uniform mat4 PV;

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

  vec3 ground = vec3(0, 0, -1);
  float t = -(dot(ground, cam) + 0) / dot(ground, dir);

  vec4 col = vec4(0,0,0,0);

  float far = gl_DepthRange.far;
  float near = gl_DepthRange.near;

  float depth = far;

  if(t > 0) {
    vec3 floorpoint = cam + t * dir;

    vec4 clip_space_pos = PV * vec4(floorpoint, 1);
    float ndc_depth = clip_space_pos.z / clip_space_pos.w;
    depth = (((far-near) * ndc_depth) + near + far) / 2.0;

    float fog = max(1 - (t * scale * 0.01), 0);

    const float c0 = 0.1;
    const float c1 = 0.15;

    float c = mix(c0, c1, checkerboard(floorpoint.xy, scale));
    c = mix((c0 + c1) * 0.5, c, fog);
    col = vec4(1,1,1, c);
  }

  FragColor = col;
  gl_FragDepth = depth;
}

)glsl";

static const float attribs[6][2] = {
    {-1, -1}, {1, -1}, {1, 1},

    {-1, -1}, {1, 1},  {-1, 1},
};

namespace g3d {

struct Skybox : public Object {
    inline static Shader *s_shader;

    ArrayBuffer m_attrib_buf;

    Skybox()
      : m_attrib_buf((void *)&attribs[0][0], sizeof(attribs), GL_ARRAY_BUFFER)
    {
        if(!s_shader) {
            s_shader =
                new Shader(viewport_vertex_shader, skybox_fragment_shader);
        }
    }

    void draw(const glm::mat4 &P, const glm::mat4 &V) override
    {
        s_shader->use();
        glBindBuffer(GL_ARRAY_BUFFER, m_attrib_buf.m_buffer);

        s_shader->setMat4("PVinv", glm::inverse(P * V));

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, 0, NULL);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(0);
    }

    void ui() override { ImGui::Checkbox("Skybox", &m_visible); }
};

struct Ground : public Object {
    inline static Shader *s_shader;

    ArrayBuffer m_attrib_buf;

    Ground(float checkersize)
      : m_attrib_buf((void *)&attribs[0][0], sizeof(attribs), GL_ARRAY_BUFFER)
      , m_checkersize(checkersize)
    {
        if(!s_shader) {
            s_shader =
                new Shader(viewport_vertex_shader, ground_fragment_shader);
        }
    }

    void draw(const glm::mat4 &P, const glm::mat4 &V) override
    {
        s_shader->use();
        glBindBuffer(GL_ARRAY_BUFFER, m_attrib_buf.m_buffer);

        s_shader->setVec3("cam", glm::inverse(V)[3]);
        s_shader->setMat4("PVinv", glm::inverse(P * V));
        s_shader->setFloat("scale", 0.5f / m_checkersize);
        s_shader->setMat4("PV", P * V);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, 0, NULL);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(0);
    }

    void ui() override
    {
        ImGui::Checkbox("Ground", &m_visible);
        ImGui::SliderFloat("CheckerSize", &m_checkersize, 10, 10000, "%.1f",
                           ImGuiSliderFlags_Logarithmic);
    }

    float m_checkersize{1000};
};

std::shared_ptr<Object>
makeSkybox()
{
    return std::make_shared<Skybox>();
}

std::shared_ptr<Object>
makeGround(float checkersize)
{
    return std::make_shared<Ground>(checkersize);
}

}  // namespace g3d
