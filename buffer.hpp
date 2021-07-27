#pragma once

#include <GL/glew.h>

namespace g3d {

struct ArrayBuffer {

  ArrayBuffer(const ArrayBuffer&) =delete;
  ArrayBuffer& operator=(const ArrayBuffer&) =delete;

  ArrayBuffer(const void *ptr, size_t len, GLenum target)
    : m_target(target)
  {
    glGenBuffers(1, &m_buffer);
    if(ptr != NULL)
      write(ptr, len);
  }

  ArrayBuffer(GLenum target)
    : m_target(target)
  {
    glGenBuffers(1, &m_buffer);
  }

  ~ArrayBuffer()
  {
    glDeleteBuffers(1, &m_buffer);
  }


  void write(const void *ptr, size_t len)
  {
    glBindBuffer(m_target, m_buffer);
    glBufferData(m_target, len, ptr, GL_STATIC_DRAW);
  }

  GLuint m_buffer;
  const GLenum m_target;

};

}
