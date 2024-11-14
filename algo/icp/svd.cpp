#include "svd.hpp"

#include <Eigen/SVD>

#include "glm_eigen.hpp"

namespace g3d {

SVD3::SVD3(const glm::mat3& m)
{
    using namespace Eigen;

    auto eigen_H = mat3_glm_to_eigen(m);
    JacobiSVD<Matrix3d> svd(eigen_H, ComputeFullU | ComputeFullV);

    U = mat3_eigen_to_glm(svd.matrixU());
    S = glm::vec3{svd.singularValues()[0], svd.singularValues()[1],
                  svd.singularValues()[2]};
    V = mat3_eigen_to_glm(svd.matrixV());
}

}
