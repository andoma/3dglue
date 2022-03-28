#include "image.hpp"

#include <stdlib.h>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#pragma GCC diagnostic ignored "-Wsign-compare"
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

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

Image2D::~Image2D() { free(m_data); }

Image2D::Image2D(size_t width, size_t height)
{
    m_width = width;
    m_height = height;
    m_data = calloc(1, width * height * 3);
}

int
Image2D::load(const char *path)
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

int
Image2D::load(const uint8_t *data, size_t len)
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

void
Image2D::save_jpeg(const char *path, int quality, bool flip)
{
    stbi_flip_vertically_on_write(flip);
    stbi_write_jpg(path, m_width, m_height, 3, m_data, quality);
}

}  // namespace g3d
