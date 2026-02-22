# 1. Timestamp Query Support

Date: 2025-10-30
Status: Accepted

## Context
High‑quality GPU performance profiling is critical for frame time optimization, regression detection, and comparative backend analysis in SnapRHI. Prior to this change, applications had no standardized, cross‑backend mechanism to measure fine‑grained GPU intervals (e.g., encoder section durations, asynchronous blit costs). Each native API (Vulkan, Metal, OpenGL/ES) exposes timestamp or counter sampling primitives with subtle differences: lifecycle management (reset vs implicit reuse), result availability semantics, units (raw tick vs nanoseconds), and resolution constraints. A unified abstraction is required to:
- Enable portable instrumentation without conditional backend code.
- Ensure consistent nanosecond results for tooling, logging, and telemetry.
- Avoid unnecessary stalls or CPU polling patterns.
- Prepare the project for external contributors adding new backends (e.g., Direct3D12, WebGPU), fostering open source adoption.

Constraints & Drivers:
- Must not degrade submission throughput (minimal additional synchronization).
- Query pools should be reusable across frames to control allocation churn.
- API should express availability vs blocking retrieval for non‑intrusive polling.
- Implementation must gracefully handle unsupported devices (feature capability flag).

## Decision
Introduce a cross‑platform timestamp query facility composed of:
1. Capability flag `QueueFamilyProperties::isTimestampQuerySupported` for discovery.
2. `QueryPoolCreateInfo` + `Device::createQueryPool` for pool allocation.
3. Command buffer method `resetQueryPool(...)` where required (Vulkan only operational; Metal/OpenGL treat as metadata/no‑op).
4. Encoder method `writeTimestamp(QueryPool*, uint32_t query)` across all encoder types (Blit, Compute, Render) to record a timestamp at a well‑defined point.
5. `QueryPool` CPU retrieval interface: availability query, blocking/non‑blocking result acquisition with unified nanosecond conversion.

Backend specifics standardize to nanoseconds and internalize native differences (reset semantics, resolve phases for Metal, conversion for Vulkan’s timestamp period). Unavailable support simply sets `isTimestampQuerySupported = false`; calls become no‑ops or return empty availability (documented behavior).

## Detailed API Changes
```cpp
struct QueueFamilyProperties {
    // ... existing properties ...
    bool isTimestampQuerySupported = false; // True iff backend exposes valid GPU timestamps
};

struct QueryPoolCreateInfo {
    uint32_t queryCount; // Number of timestamp slots managed by the pool
};

class Device {
    // ... existing methods ...
    virtual std::shared_ptr<QueryPool> createQueryPool(const QueryPoolCreateInfo& info) = 0;
};

class CommandBuffer {
    // ... existing methods ...
    virtual void resetQueryPool(snap::rhi::QueryPool* queryPool, uint32_t firstQuery, uint32_t queryCount) = 0;
};

class Encoder { // Common conceptual base for BlitEncoder, ComputeEncoder, RenderEncoder
    // ... existing methods ...
    virtual void writeTimestamp(snap::rhi::QueryPool* queryPool, uint32_t query) = 0;
};

class QueryPool {
public:
    void getAvailabilities(uint32_t firstQuery, uint32_t queryCount, std::span<bool> availabilities) {
        getResultsAndAvailabilities(firstQuery, queryCount, {}, availabilities);
    }
    void getResults(uint32_t firstQuery, uint32_t queryCount, std::span<std::chrono::nanoseconds> results) {
        getResultsAndAvailabilities(firstQuery, queryCount, results, {});
    }
    virtual void getResultsAndAvailabilities(uint32_t firstQuery,
                                             uint32_t queryCount,
                                             std::span<std::chrono::nanoseconds> results,
                                             std::span<bool> availabilities) = 0; // Backend implementation
};
```

## Backend Implementation Summary
### Vulkan
- Capability: `timestampValidBits > 0` in `VkQueueFamilyProperties`.
- Pool: `vkCreateQueryPool` with `VK_QUERY_TYPE_TIMESTAMP`.
- Reset: `vkCmdResetQueryPool(cmd, pool, firstQuery, queryCount)` each frame (required before reuse).
- Write: `vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, pool, query)` (or adjustable future stage; BOTTOM ensures all prior work).
- Retrieval: `vkGetQueryPoolResults` with `VK_QUERY_RESULT_64_BIT`; include `VK_QUERY_RESULT_WITH_AVAILABILITY_BIT` when availability requested. Conversion: `nanoseconds = raw * timestampPeriod` (timestampPeriod in ns per tick).

### Metal
- Capability: `device.counterSets.count > 0` (presence of counter sampling support). Using first counter set for timestamps.
- Pool: `MTLCounterSampleBufferDescriptor` → `newCounterSampleBufferWithDescriptor`. Maintain resolve buffer (MTLBuffer, `MTLStorageModeShared`).
- Reset: Logical no‑op; store range for later resolve.
- Write: `[encoder sampleCountersInBuffer:csb atSampleIndex:query withBarrier:YES]`.
- Submission Hook: Before `[MTLCommandBuffer commit]`, issue a `MTLBlitCommandEncoder` resolving sampled range(s) via `[blit resolveCounters:csb inRange:… destinationBuffer:… offset:…]`.
- Retrieval: After command buffer status == `MTLCommandBufferStatusCompleted`, read resolved data (already nanoseconds).

### OpenGL / OpenGL ES
- Capability: (ES 3.0 & `GL_EXT_disjoint_timer_query`) OR (Desktop GL ≥ 3.0 OR `GL_ARB_timer_query`).
- Pool: `glGenQueries(queryCount, ids)`; store GLuints.
- Reset: No‑op; writing overwrites previous timestamp.
- Write: `glQueryCounter(ids[query], GL_TIMESTAMP)`.
- Retrieval: Availability: `glGetQueryObjectiv(id, GL_QUERY_RESULT_AVAILABLE, &available)`; Results: `glGetQueryObjectui64v(id, GL_QUERY_RESULT, &value)` (already nanoseconds).

## Behavior & Semantics
- Queries represent instantaneous timestamps; duration measurement is user‑defined difference (end − start) in nanoseconds.
- `resetQueryPool` must precede Vulkan writes for a range each frame; ignored for other backends.
- Availability polling is non‑blocking except Vulkan’s optional `WAIT_BIT` (not set for pure availability calls). Blocking retrieval can be implemented by client code looping until all available, or by passing a flag in a higher‑level convenience API (future extension).
- Out‑of‑range indices are validated; failure triggers debug assert/log (release builds may ignore write).

## Alternatives Considered
1. Host CPU timing around submission (coarse, includes driver/queue latency, not GPU execution). Rejected.
2. Per‑backend custom extension usage exposed directly (complexity for app code, harms portability). Rejected.
3. Introducing timeline queries (start/end pairs automatically). Deferred; timestamp atomic writes more flexible.
4. Relying only on Vulkan timestamps (excluding Metal/GL). Rejected due to cross‑platform profiling goal.

## Consequences
Positive:
- Enables consistent frame phase breakdown (e.g., shadow pass, postprocessing) for optimization and regression alerts.
- Facilitates automated performance testing and CI trend analysis.
- Enhances open source appeal via professional profiling tooling.
- Abstraction lowers barrier to implement additional backends.

Negative / Costs:
- Additional complexity in Metal submission path (resolve step coordination).
- Slight command buffer size increase (timestamp instructions).
- Vulkan requires explicit reset; misuse can return stale data.

## Risks & Mitigations
| Risk | Mitigation |
|------|------------|
| Incorrect unit conversion (Vulkan) | Centralized conversion tested against known period; unit tests comparing deltas. |
| Missing capability detection on edge devices | Defensive feature probe; fallback sets support flag false; API calls become inert. |
| Overuse of timestamps impacting performance | Guidance in docs; optional debug warning if query density exceeds threshold. |
| Metal resolve ordering errors | Resolve executed in submission hook prior to commit; test multi‑pool, overlapping ranges. |
| OpenGL ES disjoint events (timer invalidation) | Future enhancement: expose disjoint state; for now assume driver stability; document limitation. |

## Testing Strategy
1. Unit Tests: Mock backend returning deterministic raw timestamps; verify nanosecond conversion and availability logic.
2. Integration Tests: Record start/end around known GPU workloads; assert positive duration and expected ordering across all backends.
3. Stress: High volume timestamp writes per frame (e.g., 256) validating absence of driver stalls.
4. Regression: Compare measured durations against historical baselines (per backend) within tolerance (e.g., ±10%).

## Rollout / Migration
- Phase 1: Implement Vulkan (baseline), expose flag; add internal perf instrumentation for frame phases.
- Phase 2: Implement Metal integration (resolve infrastructure). Expand test matrix.
- Phase 3: Implement OpenGL/ES support; add ES disjoint query detection (optional).
- Phase 4: Public documentation + sample code demonstrating duration measurement across encoders.

## Future Extensions
- Direct3D12 and WebGPU backend integration.
- Disjoint timestamp exposure (OpenGL) and nanosecond calibration validation harness.

## References
- Vulkan Spec: `VK_KHR_get_physical_device_properties2`, `VK_QUERY_TYPE_TIMESTAMP`.
- Metal Counter Sample API [(Apple Developer Documentation)](https://developer.apple.com/documentation/metal/creating-a-counter-sample-buffer-to-store-a-gpus-counter-data-during-a-pass?language=objc).
- OpenGL Extensions: `GL_ARB_timer_query`, `GL_EXT_disjoint_timer_query`.
