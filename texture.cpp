#include "texture.hpp"

#include <stdlib.h>

namespace g3d {

Texture2D::~Texture2D()
{
    if(m_tex)
        glDeleteTextures(1, &m_tex);
}

GLuint
Texture2D::get()
{
    if(m_img) {
        if(!m_tex)
            glGenTextures(1, &m_tex);

        glBindTexture(GL_TEXTURE_2D, m_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_img->m_width, m_img->m_height,
                     0, GL_RGB, GL_UNSIGNED_BYTE, m_img->m_data);
        m_img.reset();
    }
    return m_tex;
}
}  // namespace g3d
