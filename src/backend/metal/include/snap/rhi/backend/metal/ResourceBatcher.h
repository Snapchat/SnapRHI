//
//  Context.h
//  SnapRHI
//
//  Created by Vladyslav Deviatkov on 07.10.2020.
//  Copyright © 2021 Snapchat. All rights reserved.
//

#pragma once

#include <Metal/Metal.h>
#include <vector>

#include "snap/rhi/backend/common/Utils.hpp"

namespace snap::rhi::backend::metal {
class ResourceBatcher {
    static constexpr size_t INITIAL_BUCKET_COUNT = 2;

    struct ResourceBucket {
        static constexpr size_t INITIAL_RESOURCE_COUNT = 256;

        MTLResourceUsage usage;
        MTLRenderStages stages;
        std::vector<id<MTLResource> __unsafe_unretained> resources;

        ResourceBucket(MTLResourceUsage u, MTLRenderStages s) : usage(u), stages(s) {
            resources.reserve(INITIAL_RESOURCE_COUNT);
        }
    };

    // We rarely have more than 2-4 unique combinations per pass
    // (e.g., Read+Vert, Read+Frag, Read+All, Write+Frag)
    std::vector<ResourceBucket> _buckets;

    // CACHE: Pointer to the last used bucket.
    // This makes consecutive adds of the same type O(1) without searching.
    ResourceBucket* _lastBucket = nullptr;

public:
    ResourceBatcher() {
        _buckets.reserve(INITIAL_BUCKET_COUNT); // Pre-allocate space for common combos
    }

    SNAP_RHI_ALWAYS_INLINE void add(id<MTLResource> __unsafe_unretained resource,
                                    MTLResourceUsage usage,
                                    MTLRenderStages stages) {
        if (!resource)
            return;

        // 1. Fast Check: Are we adding to the same group as the previous call?
        if (_lastBucket && _lastBucket->usage == usage && _lastBucket->stages == stages) {
            _lastBucket->resources.push_back(resource);
            return;
        }

        // 2. Slow Check: Find existing bucket in the vector
        for (auto& bucket : _buckets) {
            if (bucket.usage == usage && bucket.stages == stages) {
                bucket.resources.push_back(resource);
                _lastBucket = &bucket; // Update cache
                return;
            }
        }

        // 3. Create new bucket if none exists
        _buckets.emplace_back(usage, stages);
        _lastBucket = &_buckets.back();
        _lastBucket->resources.push_back(resource);
    }

    SNAP_RHI_ALWAYS_INLINE void useResources(id<MTLRenderCommandEncoder> __unsafe_unretained encoder) {
        for (const auto& bucket : _buckets) {
            if (!bucket.resources.empty()) {
                [encoder useResources:bucket.resources.data()
                                count:bucket.resources.size()
                                usage:bucket.usage
                               stages:bucket.stages];
            }
        }
    }

    SNAP_RHI_ALWAYS_INLINE void useResources(id<MTLComputeCommandEncoder> __unsafe_unretained encoder) {
        for (const auto& bucket : _buckets) {
            if (!bucket.resources.empty()) {
                [encoder useResources:bucket.resources.data() count:bucket.resources.size() usage:bucket.usage];
            }
        }
    }

    void reset() {
        // We do NOT clear the _buckets vector, we only clear the contents.
        // This keeps the 'ResourceBucket' objects alive and their internal vectors reserved.
        for (auto& bucket : _buckets) {
            bucket.resources.clear();
        }
        // _lastBucket is technically invalid now, but will be reset on first 'add'
        _lastBucket = nullptr;
    }
};
} // namespace snap::rhi::backend::metal
