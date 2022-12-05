#pragma once

#include <glm/glm.hpp>

#include <xmmintrin.h>

namespace g3d {

typedef float float4 __attribute__((vector_size(16)));

float4
min(float4 a, float4 b)
{
    return _mm_min_ps(a, b);
}

float4
max(float4 a, float4 b)
{
    return _mm_max_ps(a, b);
}

float4
min(float4 a, float4 b, float4 c)
{
    return _mm_min_ps(_mm_min_ps(a, b), c);
}

float4
max(float4 a, float4 b, float4 c)
{
    return _mm_max_ps(_mm_max_ps(a, b), c);
}

float4
from(const glm::vec3 &v)
{
    return _mm_set_ps(0, v.z, v.y, v.x);
}

}  // namespace g3d
