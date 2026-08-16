#pragma once

#include "AABB.hpp"
#include "Hittable.hpp"

#include "Util/Aliases.hpp"
#include <memory>

class BVH_Node
{
private:
    AABB<f64> bounding_box_;
    std::unique_ptr<BVH_Node> left_, right_;
    // for leaf nodes
    std::size_t reference_start_{0}, reference_end_{0};
    bool is_leaf_{false};

public:
    BVH_Node(std::vector<HittableReference> &references, std::size_t start, std::size_t end)
    {
        // combine bounding box of given span
        auto combined = AABB<f64>{};
        for (auto i{start}; i < end; ++i) {
            combined = AABB{combined, references[i].bounding_box};
        }

        bounding_box_ = combined;

        auto span = end - start;

        // leaf node
        if (span <= 2) {
            is_leaf_ = true;
            reference_start_ = start;
            reference_end_ = end;
            return;
        }

        auto axis = combined.LongestAxis();

        auto comparator = [axis](const HittableReference &a, const HittableReference &b) {
            return a.bounding_box.AxisInterval(axis).lower < b.bounding_box.AxisInterval(axis).lower;
        };

        std::sort(references.begin() + start, references.begin() + end, comparator);

        auto middle = start + span / 2;
        left_ = std::make_unique<BVH_Node>(references, start, middle);
        right_ = std::make_unique<BVH_Node>(references, middle, end);
    }

    template <typename... ShapeArgs>
    std::optional<HitRecord> Hit(const dRay &ray, dInterval ray_interval,
                                 const std::vector<HittableReference> &references,
                                 const HitFunctionDispatchTable<ShapeArgs...> &dispatch) const
    {
        // if we miss this node, skip it
        // this branch is the entire season ~600 lines were changed
        if (!bounding_box_.Hit(ray, ray_interval)) return std::nullopt;

        if (is_leaf_) {
            auto result = std::optional<HitRecord>{};
            auto closest = ray_interval.upper;

            for (auto i{reference_start_}; i < reference_end_; ++i) {
                if (auto hit = dispatch.Hit(references[i], ray, dInterval{ray_interval.lower, closest})) {
                    closest = hit->distance;
                    result = hit;
                }
            }
            return result;
        }

        auto hit_left = left_->Hit(ray, ray_interval, references, dispatch);

        // skip if left has hit something near
        auto hit_right = right_->Hit(
            ray, dInterval{ray_interval.lower, hit_left ? hit_left->distance : ray_interval.upper},
            references, dispatch);

        return hit_right ? hit_right : hit_left;
    }
};

// Takes in hittable Shape arguments from an already built HittableList<ShapeArgs...> object, copies the
// list's Shape type vector pools, and sorts them into BVH Nodes
//
// std::vector<HittableReference> stores information to look up objects within type pools, and bounding box
// information for that object
//
// HitFunctionDispatchTable takes in a HittableReference and calls the Type's appropriate Hit function from
// shape_pools_, its a lookup table
//
// Complicated but saves significant time on the ray tracing hot path
template <typename... ShapeArgs>
class BoundingVolumeHierarchy
{
private:
    std::tuple<std::vector<ShapeArgs>...> shape_pools_;
    std::vector<HittableReference> references_;
    std::unique_ptr<BVH_Node> root_;
    HitFunctionDispatchTable<ShapeArgs...> dispatch_;

public:
    BoundingVolumeHierarchy(const HittableList<ShapeArgs...> &world)
        : shape_pools_{world.object_pools}, references_{world.BuildReferenceVector(world.object_pools)},
          dispatch_{shape_pools_}
    { root_ = std::make_unique<BVH_Node>(references_, 0, references_.size()); }

    BoundingVolumeHierarchy(BoundingVolumeHierarchy &&other) = delete;
    BoundingVolumeHierarchy &operator=(BoundingVolumeHierarchy &&other) = delete;

    std::optional<HitRecord> Hit(const dRay &ray, dInterval interval) const noexcept
    { return root_->Hit(ray, interval, references_, dispatch_); }
};

template <typename... ShapeArgs>
using BVH = BoundingVolumeHierarchy<ShapeArgs...>;
