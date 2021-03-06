#pragma once

#include <iostream>
#include <string>

#include "opengl.hpp"

#include <glm/glm.hpp>

namespace g3d {

struct Shader {
    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;

    Shader(const char *name, const char *header, const char *vcode,
           int vcode_len, const char *fcode, int fcode_len,
           const char *gcode = NULL, int gcode_len = 0)
    {
        GLint success;
        char err[1024];

        const char *codevec[2];
        int codelen[2];

        codevec[0] = header ?: "";
        codelen[0] = -1;

        codevec[1] = vcode;
        codelen[1] = vcode_len;

        uint32_t vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 2, codevec, codelen);
        glCompileShader(vertex);

        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if(!success) {
            glGetShaderInfoLog(vertex, sizeof(err), NULL, err);
            fprintf(stderr, "Unable to compile vertex shader: %s:\n%s\n", name,
                    err);
            exit(1);
        }

        codevec[1] = fcode;
        codelen[1] = fcode_len;

        uint32_t fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 2, codevec, codelen);
        glCompileShader(fragment);

        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if(!success) {
            glGetShaderInfoLog(fragment, sizeof(err), NULL, err);
            fprintf(stderr, "Unable to compile fragment shader: %s:\n%s\n",
                    name, err);
            exit(1);
        }

        m_id = glCreateProgram();
        glAttachShader(m_id, vertex);
        glAttachShader(m_id, fragment);

        if(gcode != NULL) {
            uint32_t geometry = glCreateShader(GL_GEOMETRY_SHADER);

            codevec[1] = gcode;
            codelen[1] = gcode_len;
            glShaderSource(geometry, 2, codevec, codelen);
            glCompileShader(geometry);

            glGetShaderiv(geometry, GL_COMPILE_STATUS, &success);
            if(!success) {
                glGetShaderInfoLog(geometry, sizeof(err), NULL, err);
                fprintf(stderr, "Unable to compile geometry shader: %s:\n%s\n",
                        name, err);
                exit(1);
            }
            glAttachShader(m_id, geometry);
        }

        glLinkProgram(m_id);
        glGetProgramiv(m_id, GL_LINK_STATUS, &success);
        if(!success) {
            glGetProgramInfoLog(m_id, sizeof(err), NULL, err);
            fprintf(stderr, "Unable to link shader: %s:\n%s\n", name, err);
            exit(1);
        }

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    ~Shader() { glDeleteProgram(m_id); }

    void use() const { glUseProgram(m_id); }

    bool has_uniform(const char *name) const
    {
        return glGetUniformLocation(m_id, name) != -1;
    }

    GLint uniloc(const char *name) const
    {
        GLint r = glGetUniformLocation(m_id, name);
        if(r == -1) {
            fprintf(stderr, "Uniform %s not found\n", name);
            //      exit(1);
        }
        return r;
    }

    GLint uniloc(const std::string &name) const { return uniloc(name.c_str()); }

    void setBool(const std::string &name, bool value) const
    {
        glUniform1i(uniloc(name), (int)value);
    }

    void setInt(const std::string &name, int value) const
    {
        glUniform1i(uniloc(name), value);
    }

    void setFloat(const std::string &name, float value) const
    {
        glUniform1f(uniloc(name), value);
    }

    void setVec2(const std::string &name, const glm::vec2 &value) const
    {
        glUniform2fv(uniloc(name), 1, &value[0]);
    }

    void setVec2(const std::string &name, float x, float y) const
    {
        glUniform2f(uniloc(name), x, y);
    }

    void setVec3(const std::string &name, const glm::vec3 &value) const
    {
        glUniform3fv(uniloc(name), 1, &value[0]);
    }

    void setVec3(const std::string &name, float x, float y, float z) const
    {
        glUniform3f(uniloc(name), x, y, z);
    }

    void setVec4(const std::string &name, const glm::vec4 &value) const
    {
        glUniform4fv(uniloc(name), 1, &value[0]);
    }

    void setVec4(const std::string &name, float x, float y, float z, float w)
    {
        glUniform4f(uniloc(name), x, y, z, w);
    }

    void setMat2(const std::string &name, const glm::mat2 &mat) const
    {
        glUniformMatrix2fv(uniloc(name), 1, GL_FALSE, &mat[0][0]);
    }

    void setMat3(const std::string &name, const glm::mat3 &mat) const
    {
        glUniformMatrix3fv(uniloc(name), 1, GL_FALSE, &mat[0][0]);
    }

    void setMat4(const std::string &name, const glm::mat4 &mat) const
    {
        glUniformMatrix4fv(uniloc(name), 1, GL_FALSE, &mat[0][0]);
    }

    unsigned int m_id;
};

}  // namespace g3d
