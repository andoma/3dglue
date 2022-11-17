#pragma once

#include <GL/glew.h>

#include <memory>
#include "image.hpp"

namespace g3d {

struct Texture2D {
    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    Texture2D() = default;

    ~Texture2D();

    GLuint get();

    void set(const std::shared_ptr<Image2D>& img) { m_img = img; }

private:
    std::shared_ptr<Image2D> m_img;

    GLuint m_tex{0};
};

}  // namespace g3d
