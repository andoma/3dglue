#pragma once

#include <assert.h>

#include <GL/glew.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>


namespace g3d {

GLenum checkGlError_(const char *file, int line);
}

#define checkGlError() checkGlError_(__FILE__, __LINE__)

#define assertGlError() assert(glGetError() == GL_NO_ERROR)
