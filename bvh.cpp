#include <xmmintrin.h>
#include <smmintrin.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/intersect.hpp>

#include <vector>
#include <algorithm>
#include <thread>
#include <atomic>

#include "vertexbuffer.hpp"
#include "bvh.hpp"

namespace g3d {

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
    glm::vec3 inv_direction;
};

struct HitRecord {
    int index{-1};
    float distance{INFINITY};
    glm::vec2 bc;
};

inline __m128
loadFloat3(const glm::vec3& pos)
{
    return _mm_set_ps(0, pos.z, pos.y, pos.x);
}

struct AABB {
    inline bool hit(__m128 origin, __m128 idir) const
    {
        __m128 min = loadFloat3(m_min);
        __m128 max = loadFloat3(m_max);
        min = (min - origin) * idir;
        max = (max - origin) * idir;
        __m128 mi = _mm_min_ps(min, max);
        __m128 ma = _mm_max_ps(min, max);

        float tmax = glm::min(glm::min(ma[0], ma[1]), ma[2]);
        if(tmax < 0) {
            return false;
        }

        float tmin = glm::max(glm::max(mi[0], mi[1]), mi[2]);
        if(tmin > tmax) {
            return false;
        }
        return true;
    }

    inline AABB operator+(const AABB& o)
    {
        return AABB{glm::min(m_min, o.m_min), glm::max(m_max, o.m_max)};
    }

    glm::vec3 m_min{NAN};
    glm::vec3 m_max{NAN};
};

struct BvhNode {
    int left;
    int right;
    AABB left_box;
    AABB right_box;
};

template <typename T>
class BVH : public T {
public:
    int build(std::vector<int> primitives, int axis, bool* run)
    {
        BvhNode bn;
        if(primitives.size() <= 2) {
            bn.left = -this->push(primitives[0]);
            bn.left_box = this->aabb(primitives[0]);

            if(primitives.size() == 2) {
                bn.right = -this->push(primitives[1]);
                bn.right_box = this->aabb(primitives[1]);
            } else {
                bn.right = bn.left;
                bn.right_box = bn.left_box;
            }

        } else if(*run) {
            std::sort(primitives.begin(), primitives.end(),
                      [&](const auto& a, const auto& b) {
                          return this->get_position(a, 2 - axis) <
                                 this->get_position(b, 2 - axis);
                      });

            bn.left = build(
                std::vector<int>(primitives.begin(),
                                 primitives.begin() + primitives.size() / 2),
                (axis + 1) % 3, run);
            bn.right = build(
                std::vector<int>(primitives.begin() + primitives.size() / 2,
                                 primitives.end()),
                (axis + 1) % 3, run);

            bn.left_box =
                m_nodes[bn.left].left_box + m_nodes[bn.left].right_box;
            bn.right_box =
                m_nodes[bn.right].left_box + m_nodes[bn.right].right_box;
        }

        size_t r = m_nodes.size();
        m_nodes.push_back(bn);
        return (int)r;
    }

    void hit(const Ray& ray, HitRecord& rec, int index, __m128 origin,
             __m128 invD) const
    {
        if(index < 0) {
            // Negative index, intersecting a primitive
            this->hit_primitive(-index, ray, rec);
            return;
        }

        const auto& n = m_nodes[index];

        if(n.left_box.hit(origin, invD)) {
            hit(ray, rec, n.left, origin, invD);
        }
        if(n.right_box.hit(origin, invD)) {
            hit(ray, rec, n.right, origin, invD);
        }
    }

    std::vector<BvhNode> m_nodes;
};

class Points {
    float m_radii{1};
    float m_radii_sq{m_radii * m_radii};

protected:
    int push(int primitive) { return primitive; }

    void hit_primitive(int primitive, const Ray& ray, HitRecord& rec) const
    {
        float distance;
        if(!glm::intersectRaySphere(ray.origin, ray.direction, point(primitive),
                                    m_radii_sq, distance))
            return;

        if(distance < rec.distance) {
            rec.index = primitive;
            rec.distance = distance;
        }
    }

    AABB aabb(int primitive) const
    {
        auto center = point(primitive);
        glm::vec3 d{m_radii};
        return AABB{center - d, center + d};
    }

    float get_position(int primitive, int axis) const
    {
        const float* pos = m_vb->get_attributes(VertexAttribute::Position);
        pos += primitive * m_vb->get_stride(VertexAttribute::Position);
        return pos[axis];
    }

public:
    inline glm::vec3 point(int primitive) const
    {
        const float* pos = m_vb->get_attributes(VertexAttribute::Position);
        pos += primitive * m_vb->get_stride(VertexAttribute::Position);
        return glm::vec3{pos[0], pos[1], pos[2]};
    }

    std::shared_ptr<VertexBuffer> m_vb;
};

struct PointIntersector : public Intersector {
    BVH<Points> m_bvh;

    std::thread m_thread;

    bool m_run{true};
    std::atomic<int> m_start{-1};

    ~PointIntersector()
    {
        m_run = false;
        m_thread.join();
    }

    PointIntersector(const std::shared_ptr<VertexBuffer>& vb)
    {
        m_bvh.m_vb = vb;
        if(vb->size() == 0)
            return;
        m_thread = std::thread([&]() {
            std::vector<int> primitives;
            size_t size = m_bvh.m_vb->size();
            primitives.reserve(size);
            for(size_t i = 0; i < size; i++) {
                primitives.push_back(i);
            }
            m_start = m_bvh.build(primitives, 2, &m_run);
        });
    }

    std::optional<std::pair<size_t, glm::vec3>> intersect(
        const glm::vec3& origin, const glm::vec3& direction) const override
    {
        int start = m_start.load();
        if(start == -1)
            return std::nullopt;
        Ray ray{origin, direction, 1.0f / direction};
        HitRecord rec;
        m_bvh.hit(ray, rec, start, loadFloat3(origin),
                  loadFloat3(ray.inv_direction));

        if(rec.index == -1)
            return std::nullopt;
        return std::make_pair(rec.index, m_bvh.point(rec.index));
    };
};

class Triangles {
protected:
    int push(int primitive) { return primitive; }

    void hit_primitive(int primitive, const Ray& ray, HitRecord& rec) const
    {
        float distance;
        glm::vec2 bc;
        const auto t = triangle(primitive);

        if(!glm::intersectRayTriangle(ray.origin, ray.direction, t[0], t[1],
                                      t[2], bc, distance))
            return;

        if(distance < rec.distance) {
            rec.index = primitive;
            rec.distance = distance;
            rec.bc = bc;
        }
    }

    AABB aabb(int primitive) const
    {
        const auto tri = triangle(primitive);
        return AABB{glm::min(glm::min(tri[0], tri[1]), tri[2]),
                    glm::max(glm::max(tri[0], tri[1]), tri[2])};
    }

    float get_position(int primitive, int axis) const
    {
        const auto v = (*m_ib)[primitive];
        const size_t stride = m_vb->get_stride(VertexAttribute::Position);
        const float* pos = m_vb->get_attributes(VertexAttribute::Position);
        pos += v.x * stride;
        return pos[axis];
    }

public:
    inline glm::vec3 point(int primitive, const glm::vec2& bc) const
    {
        glm::vec3 abc{1.0f - bc.x - bc.y, bc.x, bc.y};
        return triangle(primitive) * abc;
    }

    inline glm::mat3 triangle(int primitive) const
    {
        const auto v = (*m_ib)[primitive];
        const size_t stride = m_vb->get_stride(VertexAttribute::Position);

        const float* p = m_vb->get_attributes(VertexAttribute::Position);
        const float* p0 = p + v.x * stride;
        const float* p1 = p + v.y * stride;
        const float* p2 = p + v.z * stride;

        return glm::mat3{glm::vec3{p0[0], p0[1], p0[2]},
                         glm::vec3{p1[0], p1[1], p1[2]},
                         glm::vec3{p2[0], p2[1], p2[2]}};
    }

    std::shared_ptr<VertexBuffer> m_vb;
    std::shared_ptr<std::vector<glm::ivec3>> m_ib;
};

struct TriangleIntersector : public Intersector {
    BVH<Triangles> m_bvh;

    std::thread m_thread;

    bool m_run{true};
    std::atomic<int> m_start{-1};

    ~TriangleIntersector()
    {
        m_run = false;
        m_thread.join();
    }

    TriangleIntersector(const std::shared_ptr<VertexBuffer>& vb,
                        const std::shared_ptr<std::vector<glm::ivec3>>& ib)
    {
        m_bvh.m_vb = vb;
        m_bvh.m_ib = ib;
        if(vb->size() == 0 || ib->size() == 0)
            return;
        m_thread = std::thread([&]() {
            std::vector<int> primitives;
            size_t size = m_bvh.m_ib->size();
            primitives.reserve(size);
            for(size_t i = 0; i < size; i++) {
                primitives.push_back(i);
            }
            m_start = m_bvh.build(primitives, 2, &m_run);
        });
    }

    std::optional<std::pair<size_t, glm::vec3>> intersect(
        const glm::vec3& origin, const glm::vec3& direction) const override
    {
        int start = m_start.load();
        if(start == -1)
            return std::nullopt;
        Ray ray{origin, direction, 1.0f / direction};
        HitRecord rec;
        m_bvh.hit(ray, rec, start, loadFloat3(origin),
                  loadFloat3(ray.inv_direction));

        if(rec.index == -1)
            return std::nullopt;
        return std::make_pair(rec.index, m_bvh.point(rec.index, rec.bc));
    };
};



std::shared_ptr<Intersector>
Intersector::make(const std::shared_ptr<VertexBuffer>& vb,
                  IntersectionMode mode)
{
    return std::make_shared<PointIntersector>(vb);
}

std::shared_ptr<Intersector>
Intersector::make(const std::shared_ptr<VertexBuffer>& vb,
                  const std::shared_ptr<std::vector<glm::ivec3>>& ib)
{
    return std::make_shared<TriangleIntersector>(vb, ib);
}

}  // namespace g3d
