#include "snap/rhi/backend/metal/CommandEncoder.hpp"
#include "snap/rhi/Common.h"
#include "snap/rhi/backend/common/Utils.hpp"
#include "snap/rhi/backend/metal/Buffer.h"
#include "snap/rhi/backend/metal/CommandBuffer.h"
#include "snap/rhi/backend/metal/Device.h"
#include "snap/rhi/backend/metal/Texture.h"
#include <Metal/Metal.h>

#include "snap/rhi/common/OS.h"

namespace snap::rhi::backend::metal {
void writeTimestamp(snap::rhi::CommandBuffer* commandBuffer,
                    const id<MTLCommandEncoder>& encoder,
                    snap::rhi::QueryPool* queryPool,
                    uint32_t query,
                    const snap::rhi::TimestampLocation location) {
    // TODO: Implement
}
} // namespace snap::rhi::backend::metal
