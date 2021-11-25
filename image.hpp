#pragma once

#include <GL/glew.h>

namespace g3d {

struct Image2D {

  Image2D(const Image2D&) =delete;
  Image2D& operator=(const Image2D&) =delete;

  // Load encoded image from file
  Image2D(const char *path);

  // Load encoded image from memory
  Image2D(const uint8_t *data, size_t len);

  // This takes ownership of *data
  Image2D(void **data, size_t width, size_t height)
    : m_data(*data)
    , m_width(width)
    , m_height(height)
  {
    *data = NULL;
  }

  ~Image2D();

  void *m_data{NULL};
  size_t m_width{0};
  size_t m_height{0};

private:

  int load(const char *path);

  int load(const uint8_t *data, size_t len);


};

}
