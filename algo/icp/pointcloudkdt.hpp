#pragma once

#include "nanoflann.hpp"
#include "svd.hpp"
#include "thread_pool.hpp"

#include "3dglue/vertexbuffer.hpp"

#include <mutex>

namespace g3d {

struct NNResult {
    double m_err_sqr{0};
    size_t m_num_corres{0};

    NNResult& operator+=(const NNResult& o)
    {
        m_err_sqr += o.m_err_sqr;
        m_num_corres += o.m_num_corres;
        return *this;
    }

    double rmse() const
    {
        return sqrt(m_err_sqr / m_num_corres);
    }
};

struct Corresponence {
    size_t ref_idx;
    float dist_sqr;

    bool operator<(const Corresponence& o) const
    {
        return dist_sqr < o.dist_sqr;
    }
};

struct PointCloudKDT {
    PointCloudKDT(const std::shared_ptr<g3d::VertexBuffer>& pc) : m_pc(pc)
    {
    }

    typedef nanoflann::KDTreeSingleIndexAdaptor<
        nanoflann::L2_Simple_Adaptor<float, PointCloudKDT>, PointCloudKDT, 3>
        kd_tree_t;

    const std::shared_ptr<g3d::VertexBuffer> m_pc;

    inline size_t kdtree_get_point_count() const
    {
        return m_pc->size();
    }

    inline float kdtree_get_pt(const size_t idx, const size_t dim) const
    {
        const float* pos = m_pc->get_attributes(VertexAttribute::Position);
        pos += idx * m_pc->get_stride(VertexAttribute::Position);
        return pos[dim];
    }

    template <class BBOX>
    bool kdtree_get_bbox(BBOX& /* bb */) const
    {
        return false;
    }
    kd_tree_t m_kd{3, *this, nanoflann::KDTreeSingleIndexAdaptorParams(10)};

    std::pair<float, size_t> find_nn(const glm::vec3& v) const
    {
        const size_t num_results = 1;
        size_t ret_index;
        float out_dist_sqr;
        nanoflann::KNNResultSet<float> resultSet(num_results);
        resultSet.init(&ret_index, &out_dist_sqr);
        m_kd.findNeighbors(resultSet, &v[0], nanoflann::SearchParams());
        return std::make_pair(out_dist_sqr, ret_index);
    }
};

static inline NNResult find_nn(const PointCloudKDT& ref,
                               const glm::mat4& transform,
                               const VertexBuffer& src,
                               float max_correspondence_distance_sqr,
                               std::vector<Corresponence>& result,
                               thread_pool& tp)
{
    std::mutex mutex;

    double error2_acc = 0;
    size_t num_corres_acc = 0;

    tp.parallelize_loop(
        0l, src.size(), [&](const long& start, const long& end) {
            float error2 = 0.0;
            size_t num_corres = 0;
            for (long i = start; i < end; i++) {
                auto p = transform * glm::vec4{src.position(i), 1};
                nanoflann::KNNResultSet<float> resultSet(1);
                resultSet.init(&result[i].ref_idx, &result[i].dist_sqr);

                ref.m_kd.findNeighbors(resultSet, &p[0],
                                       nanoflann::SearchParams());
                if (result[i].dist_sqr > max_correspondence_distance_sqr)
                    continue;
                num_corres++;
                error2 += result[i].dist_sqr;
            }
            mutex.lock();
            error2_acc += error2;
            num_corres_acc += num_corres;
            mutex.unlock();
        });

    return NNResult{error2_acc, num_corres_acc};
}

static inline glm::mat4 umeyama(const VertexBuffer& ref, const glm::mat4& t,
                                const VertexBuffer& src,
                                const std::vector<Corresponence>& corr,
                                float max_correspondence_distance_sqr,
                                size_t num_corr)
{
    glm::vec3 ref_centroid{0};
    glm::vec3 our_centroid{0};

    for (size_t i = 0; i < corr.size(); i++) {
        if (corr[i].dist_sqr > max_correspondence_distance_sqr)
            continue;

        const glm::vec3 p = t * glm::vec4{src.position(i), 1};
        our_centroid += p;
        ref_centroid += ref.position(corr[i].ref_idx);
    }

    our_centroid /= glm::vec3{(float)num_corr};
    ref_centroid /= glm::vec3{(float)num_corr};

    glm::mat3 H{0};

    for (size_t i = 0; i < corr.size(); i++) {
        if (corr[i].dist_sqr > max_correspondence_distance_sqr)
            continue;

        const glm::vec3 p = t * glm::vec4{src.position(i), 1};
        auto o_pt = p - our_centroid;
        auto r_pt = ref.position(corr[i].ref_idx) - ref_centroid;
        for (int j = 0; j < 3; j++) {
            H[j].x += o_pt[j] * r_pt[0];
            H[j].y += o_pt[j] * r_pt[1];
            H[j].z += o_pt[j] * r_pt[2];
        }
    }

    SVD3 svd(H);

    glm::mat3 s{glm::determinant(svd.U) * glm::determinant(svd.V) < 0 ? -1.0f
                                                                      : 1.0f};

    auto R = svd.U * s * glm::transpose(svd.V);
    auto T = ref_centroid - R * our_centroid;
    return glm::mat4{glm::vec4{R[0], 0}, glm::vec4{R[1], 0}, glm::vec4{R[2], 0},
                     glm::vec4{T, 1}} *
           t;
}

};
