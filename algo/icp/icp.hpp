#include <3dglue/object.hpp>

class thread_pool;

namespace g3d {

struct ICP {
    virtual ~ICP() {}
    virtual double step() = 0;
};


std::unique_ptr<ICP> make_ICP(std::shared_ptr<g3d::VertexBuffer> vertices,
                              std::shared_ptr<g3d::VertexBuffer> reference,
                              std::shared_ptr<g3d::Object> obj,
                              thread_pool& tp);
}
