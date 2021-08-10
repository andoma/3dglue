#pragma once

#include <GL/glew.h>

namespace g3d {

struct Texture2D {

  Texture2D(const Texture2D&) =delete;
  Texture2D& operator=(const Texture2D&) =delete;

  Texture2D(const char *path);

  Texture2D(void **data, size_t width, size_t height)
    : m_data(*data)
    , m_width(width)
    , m_height(height)
  {
    *data = NULL;
  }

  ~Texture2D();

  GLuint get();

  void load(const char *path);

  GLuint m_tex{0};

  bool m_dirty{false};
  void *m_data{NULL};
  size_t m_width{0};
  size_t m_height{0};

};

}
