#include "icp.hpp"

#include "pointcloudkdt.hpp"

namespace g3d {

struct SingleICP : public ICP {
    SingleICP(const std::shared_ptr<g3d::VertexBuffer>& src,
              const std::shared_ptr<g3d::VertexBuffer>& reference,
              const std::shared_ptr<g3d::Object>& object,
              thread_pool& tp) :
        m_src(src),
        m_reference(reference),
        m_object(object),
        m_tp(tp),
        m_kds(std::make_unique<PointCloudKDT>(m_reference))
    {
    }

    double step() override;

    const std::shared_ptr<g3d::VertexBuffer> m_src;
    const std::shared_ptr<g3d::VertexBuffer> m_reference;
    const std::shared_ptr<g3d::Object> m_object;
    thread_pool& m_tp;
    std::unique_ptr<PointCloudKDT> m_kds;

    std::vector<Corresponence> m_result;

    float m_d{100};
};

double SingleICP::step()
{
    const double max_correspondence_distance = m_d;

    m_d--;
    if (m_d < 10)
        m_d = 10;

    const double max_correspondence_distance_sqr =
        max_correspondence_distance * max_correspondence_distance;

    const glm::mat4 mat = m_object->m_model_matrix;

    m_result.resize(m_src->size());

    auto nnr = find_nn(*m_kds, mat, *m_src, max_correspondence_distance_sqr,
                       m_result, m_tp);
    if (nnr.m_num_corres < 100)
        return NAN;

    auto upd = umeyama(*m_reference, mat, *m_src, m_result,
                       max_correspondence_distance_sqr, nnr.m_num_corres);
    m_object->m_model_matrix = upd;
    return nnr.rmse();
}

std::unique_ptr<ICP> make_ICP(std::shared_ptr<g3d::VertexBuffer> vertices,
                              std::shared_ptr<g3d::VertexBuffer> reference,
                              std::shared_ptr<g3d::Object> obj,
                              thread_pool& tp)
{
    return std::make_unique<SingleICP>(vertices, reference, obj, tp);
}

}
