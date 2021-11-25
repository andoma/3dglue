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

Image2D::Image2D(const char *path)
{
  if(load(path))
    throw std::runtime_error(stbi_failure_reason());
}

Image2D::Image2D(const uint8_t *data, size_t len)
{
  if(load(data, len))
    throw std::runtime_error(stbi_failure_reason());
}

Image2D::~Image2D()
{
  free(m_data);
}

int Image2D::load(const char *path)
{
  int width;
  int height;
  int channels;
  m_data = stbi_load(path, &width, &height, &channels, 3);
  if(m_data == NULL) {
    return -1;
  }

  m_width = width;
  m_height = height;
  return 0;
}

int Image2D::load(const uint8_t *data, size_t len)
{
  int width;
  int height;
  int channels;

  m_data = stbi_load_from_memory(data, len, &width, &height, &channels, 3);
  if(m_data == NULL) {
    return -1;
  }

  m_width = width;
  m_height = height;
  return 0;
}

}
