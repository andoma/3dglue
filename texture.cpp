#include "texture.hpp"

#include <stdlib.h>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#pragma GCC diagnostic ignored "-Wsign-compare"
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
#include "stb_image.h"

namespace g3d {

Texture2D::Texture2D(const char *path)
{
  if(load(path))
    throw std::runtime_error("Unable to load texture");
}

Texture2D::Texture2D(const uint8_t *data, size_t len)
{
  if(load(data, len))
    throw std::runtime_error("Unable to load texture");
}

Texture2D::~Texture2D()
{
  if(m_tex)
    glDeleteTextures(1, &m_tex);
  free(m_data);
}


GLuint Texture2D::get()
{
  if(!m_tex)
    glGenTextures(1, &m_tex);

  if(m_dirty) {
    glBindTexture(GL_TEXTURE_2D, m_tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB,
                 m_width,
                 m_height,
                 0,
                 GL_RGB,
                 GL_UNSIGNED_BYTE,
                 m_data);
      m_dirty = false;
  }
  return m_tex;
}


int Texture2D::load(const char *path)
{
  int width;
  int height;
  int channels;
  m_data = stbi_load(path, &width, &height, &channels, 3);
  if(m_data == NULL) {
    return -1;
  }

  m_dirty = true;
  m_width = width;
  m_height = height;
  return 0;
}

int Texture2D::load(const uint8_t *data, size_t len)
{
  int width;
  int height;
  int channels;

  m_data = stbi_load_from_memory(data, len, &width, &height, &channels, 3);
  if(m_data == NULL) {
    return -1;
  }

  m_dirty = true;
  m_width = width;
  m_height = height;
  return 0;
}

}
