#pragma once

static inline Eigen::Matrix4d mat4_glm_to_eigen(const glm::mat4& src)
{
    Eigen::Matrix4d dst;
    for (int i = 0; i < 4; i++) {
        dst(0, i) = src[i].x;
        dst(1, i) = src[i].y;
        dst(2, i) = src[i].z;
        dst(3, i) = src[i].w;
    }
    return dst;
}

static inline glm::mat4 mat4_eigen_to_glm(const Eigen::Matrix4d& src)
{
    return glm::mat4{{src(0, 0), src(1, 0), src(2, 0), src(3, 0)},
                     {src(0, 1), src(1, 1), src(2, 1), src(3, 1)},
                     {src(0, 2), src(1, 2), src(2, 2), src(3, 2)},
                     {src(0, 3), src(1, 3), src(2, 3), src(3, 3)}};
}

static inline Eigen::Matrix3d mat3_glm_to_eigen(const glm::mat3& src)
{
    Eigen::Matrix3d dst;
    for (int i = 0; i < 3; i++) {
        dst(0, i) = src[i].x;
        dst(1, i) = src[i].y;
        dst(2, i) = src[i].z;
    }
    return dst;
}

static inline Eigen::Matrix3d mat3_glm_to_eigen(const glm::dmat3& src)
{
    Eigen::Matrix3d dst;
    for (int i = 0; i < 3; i++) {
        dst(0, i) = src[i].x;
        dst(1, i) = src[i].y;
        dst(2, i) = src[i].z;
    }
    return dst;
}

static inline glm::mat3 mat3_eigen_to_glm(const Eigen::Matrix3d& src)
{
    return glm::mat3{{src(0, 0), src(1, 0), src(2, 0)},
                     {src(0, 1), src(1, 1), src(2, 1)},
                     {src(0, 2), src(1, 2), src(2, 2)}};
}
