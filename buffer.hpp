#pragma once

#include <GL/glew.h>

namespace g3d {

struct ArrayBuffer {

  ArrayBuffer(const ArrayBuffer&) =delete;
  ArrayBuffer& operator=(const ArrayBuffer&) =delete;

  ArrayBuffer(const void *ptr, size_t len)
  {
    glGenBuffers(1, &m_buffer);
    if(ptr != NULL)
      write(ptr, len);
  }

  ArrayBuffer()
  {
    glGenBuffers(1, &m_buffer);
  }

  ~ArrayBuffer()
  {
    glDeleteBuffers(1, &m_buffer);
  }


  void write(const void *ptr, size_t len)
  {
    glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
    glBufferData(GL_ARRAY_BUFFER, len, ptr, GL_STATIC_DRAW);
  }

  GLuint m_buffer;

};

}
