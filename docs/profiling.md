# SnapRHI Profiling Guide

This guide covers performance profiling for SnapRHI — including SnapRHI's built-in GPU timestamp queries, custom profiling hooks, memory tracking, and platform-specific tools.

> **Related:** [Performance Design](performance.md) | [Build Guide](build.md) | [Debugging Guide](debugging.md) | [Resource Management](resource-management.md) | [ADR-0002 — Timestamp Query Support](adr/0002-timestamp-query-support.md)

---

## Table of Contents

1. [Profiling Principles](#1-profiling-principles)
2. [Build Configuration for Profiling](#2-build-configuration-for-profiling)
3. [GPU Timestamp Queries](#3-gpu-timestamp-queries)
4. [Custom Profiling Labels](#4-custom-profiling-labels)
5. [Memory Tracking](#5-memory-tracking)
6. [Platform Tools](#6-platform-tools)
7. [GPU Profiling with External Tools](#7-gpu-profiling-with-external-tools)
8. [Collecting Artifacts](#8-collecting-artifacts)

---

## 1. Profiling Principles

### 1.1 Methodology

1. **Sample first** — Use sampling profilers to find hotspots
2. **Instrument second** — Add detailed timing only where needed
3. **Profile representative workloads** — Use production-like scenes
4. **Minimize noise** — Close background apps, disable frequency scaling
5. **Compare multiple runs** — Account for variance

### 1.2 Bottleneck Categories

| Category | Symptoms | Tools |
|----------|----------|-------|
| **CPU-bound** | High CPU usage, low GPU utilization | CPU profilers, flame graphs |
| **GPU-bound** | Low CPU usage, high frame time | GPU timestamp queries, frame capture |
| **Memory-bound** | Allocation spikes, fragmentation | `captureMemorySnapshot()`, heap analysis |
| **Transfer-bound** | High copy times, stalls | Timestamp queries around blit encoders |

---

## 2. Build Configuration for Profiling

### 2.1 Recommended Settings

For accurate profiling, **disable debug overhead** while keeping symbols:

```bash
# Using CMake presets — release builds disable labels/logs by default
cmake --preset macos-metal-release
cmake --build build/macos-metal-release

# Using build.sh
./build.sh --metal --release

# Manual CMake — explicitly disable debug features
cmake -B build \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DSNAP_RHI_ENABLE_METAL=ON \
    -DSNAP_RHI_ENABLE_DEBUG_LABELS=OFF \
    -DSNAP_RHI_ENABLE_LOGS=OFF \
    -DSNAP_RHI_ENABLE_ALL_VALIDATION=OFF
```

### 2.2 Build Type Guidance

| Use Case | Build Type | SnapRHI Flags | Notes |
|----------|------------|---------------|-------|
| CPU profiling | `RelWithDebInfo` | Debug labels OFF, Logs OFF, Validation OFF | Optimized + symbols |
| GPU profiling | `Release` or `RelWithDebInfo` | Debug labels OFF, Validation OFF | Match production |
| Memory profiling | `RelWithDebInfo` | Debug labels ON (for resource names) | Labels help identify resources |
| Custom profiling hooks | Any | `SNAP_RHI_ENABLE_CUSTOM_PROFILING_LABELS=ON` | Adds profiling scope callbacks |

### 2.3 What to Avoid

| Avoid | Reason |
|-------|--------|
| Sanitizers during profiling | Change timing significantly (ASan: 2-3×, TSan: 5-15×) |
| Debug builds for GPU profiling | Driver behavior differs; unoptimized CPU code hides real GPU bottlenecks |
| Validation layers (`SNAP_RHI_ENABLE_ALL_VALIDATION`) | Adds substantial CPU overhead on every API call |
| Debug labels for CPU timing | `SNAP_RHI_ENABLE_DEBUG_LABELS=ON` forwards labels to native API (Metal/GL/Vulkan), adding overhead |

---

## 3. GPU Timestamp Queries

SnapRHI provides a cross-backend timestamp query system for measuring GPU execution time. See [ADR-0002](adr/0002-timestamp-query-support.md) for the design rationale.

### 3.1 Capability Check

Timestamp queries are not available on all devices. Check before use:

```cpp
const auto& caps = device->getCapabilities();
auto* queue = device->getCommandQueue(0, 0);

// Check queue family supports timestamp queries
bool supported = caps.queueFamilyProperties[0].isTimestampQuerySupported;
```

### 3.2 Creating a QueryPool

```cpp
auto queryPool = device->createQueryPool({.queryCount = 64});
```

### 3.3 Recording Timestamps

Timestamps can be recorded from **any encoder type** (render, compute, blit). The `TimestampLocation` parameter specifies whether the timestamp marks the beginning or end of a measurement region:

```cpp
auto commandBuffer = device->createCommandBuffer({.commandQueue = queue});

// Reset queries before reuse (required for Vulkan; no-op on Metal/OpenGL)
commandBuffer->resetQueryPool(queryPool.get(), 0, 4);

// Render pass timing
auto renderEncoder = commandBuffer->getRenderCommandEncoder();
renderEncoder->beginEncoding(renderPassInfo);
renderEncoder->writeTimestamp(queryPool.get(), 0, snap::rhi::TimestampLocation::Start);
// ... draw calls ...
renderEncoder->writeTimestamp(queryPool.get(), 1, snap::rhi::TimestampLocation::End);
renderEncoder->endEncoding();

// Compute pass timing
auto computeEncoder = commandBuffer->getComputeCommandEncoder();
computeEncoder->beginEncoding();
computeEncoder->writeTimestamp(queryPool.get(), 2, snap::rhi::TimestampLocation::Start);
// ... dispatch calls ...
computeEncoder->writeTimestamp(queryPool.get(), 3, snap::rhi::TimestampLocation::End);
computeEncoder->endEncoding();
```

**`TimestampLocation`:**

| Value | Guarantee |
|-------|-----------|
| `Start` | Timestamp recorded *before* subsequent work begins |
| `End` | Timestamp recorded *after* all previous work has finished |

### 3.4 Retrieving Results

After GPU completion (wait on fence), read results:

```cpp
fence->waitForComplete();

// Simple retrieval
std::array<std::chrono::nanoseconds, 4> timestamps;
auto result = queryPool->getResults(0, 4, timestamps);

if (result == snap::rhi::QueryPool::Result::Available) {
    auto renderDuration = timestamps[1] - timestamps[0];
    auto computeDuration = timestamps[3] - timestamps[2];
    std::cout << "Render: " << renderDuration.count() << " ns\n";
    std::cout << "Compute: " << computeDuration.count() << " ns\n";
}
```

**Retrieval with per-query availability:**

```cpp
std::array<std::chrono::nanoseconds, 4> timestamps;
std::array<bool, 4> available;
auto result = queryPool->getResultsAndAvailabilities(0, 4, timestamps, available);

for (int i = 0; i < 4; ++i) {
    if (available[i]) {
        std::cout << "Query " << i << ": " << timestamps[i].count() << " ns\n";
    }
}
```

### 3.5 Result Codes

| Result | Meaning | Action |
|--------|---------|--------|
| `Available` | All queries finished; timing data valid | Use the values |
| `NotReady` | GPU still processing | Retry later |
| `Disjoint` | Timing data invalid (GPU throttled, context switched) | Discard — do not include in averages |
| `Error` | Invalid query ID or internal failure | Check query range |

### 3.6 Backend Implementation Details

| Aspect | Vulkan | Metal | OpenGL / OpenGL ES |
|--------|--------|-------|-------------------|
| **Capability** | `timestampValidBits > 0` in queue family | Counter sets available | Desktop GL ≥ 3.0 or `GL_ARB_timer_query`; ES 3.0 + `GL_EXT_disjoint_timer_query` |
| **Pool creation** | `vkCreateQueryPool` (`VK_QUERY_TYPE_TIMESTAMP`) | `MTLCounterSampleBuffer` + resolve buffer | `glGenQueries` |
| **Reset** | `vkCmdResetQueryPool` (required each frame) | No-op | No-op |
| **Write** | `vkCmdWriteTimestamp` | `sampleCountersInBuffer:atSampleIndex:withBarrier:` | `glQueryCounter(GL_TIMESTAMP)` |
| **Retrieval** | `vkGetQueryPoolResults` → convert via `timestampPeriod` | Read resolved buffer (already nanoseconds) | `glGetQueryObjectui64v` (already nanoseconds) |
| **Units** | Raw ticks → multiplied by `timestampPeriod` → nanoseconds | Nanoseconds natively | Nanoseconds natively |

### 3.7 Best Practices

- **Reset before reuse:** Always call `commandBuffer->resetQueryPool()` before writing to queries in a new frame (required for Vulkan, no-op elsewhere)
- **Don't over-instrument:** Excessive timestamps can impact GPU scheduling. Measure at the pass level, not per-draw-call
- **Handle `Disjoint`:** On mobile (especially OpenGL ES), GPU throttling can invalidate timing data. Discard disjoint results from averages
- **Frame-delay results:** Read query results from N-2 frames to avoid stalling the GPU pipeline
- **Use pairs:** Always record `Start` and `End` timestamps as a pair for accurate duration measurement

---

## 4. Custom Profiling Labels

SnapRHI supports integration with external profiling tools (Tracy, Perfetto, etc.) via custom profiling callbacks.

### 4.1 Enable at Build Time

```bash
cmake -B build -DSNAP_RHI_ENABLE_CUSTOM_PROFILING_LABELS=ON -DSNAP_RHI_ENABLE_METAL=ON
```

### 4.2 Configure Callbacks

When `SNAP_RHI_ENABLE_CUSTOM_PROFILING_LABELS` is enabled, provide callbacks in `DeviceCreateInfo`:

```cpp
snap::rhi::DeviceCreateInfo deviceInfo{};
deviceInfo.profilingCreateInfo = snap::rhi::ProfilingCreateInfo{
    .onStartScope = [](std::string_view label) {
        // Integration point: start a profiling zone
        // e.g., Tracy: ZoneTransientN(___tracy_scoped_zone, label.data(), true);
        // e.g., Perfetto: TRACE_EVENT_BEGIN("snaprhi", perfetto::DynamicString(label));
        myProfiler::beginScope(label);
    },
    .onEndScope = [](std::string_view label) {
        // Integration point: end a profiling zone
        myProfiler::endScope(label);
    }
};
```

### 4.3 What Gets Instrumented

SnapRHI wraps internal operations with `ProfilingScope` RAII guards. When callbacks are set, you'll see scope entries for:

- Device resource creation operations
- Command buffer encoding phases
- Queue submission
- Backend-specific internal operations

### 4.4 Overhead

When `SNAP_RHI_ENABLE_CUSTOM_PROFILING_LABELS=OFF` (default), the `ProfilingScope` class and all callback infrastructure is compiled out completely — zero overhead.

When enabled, overhead is determined by your callback implementation. Keep callbacks lightweight (nanosecond-scale) to avoid distorting measurements.

---

## 5. Memory Tracking

SnapRHI provides built-in memory usage tracking for all GPU resources.

### 5.1 Per-Resource Memory Usage

Every `DeviceChild` reports estimated memory usage:

```cpp
auto buffer = device->createBuffer(bufferInfo);
uint32_t cpuBytes = buffer->getCPUMemoryUsage();  // CPU-side tracking structures
uint32_t gpuBytes = buffer->getGPUMemoryUsage();   // GPU allocation estimate
```

### 5.2 Device-Level Aggregation

Query total memory usage across all live resources:

```cpp
uint64_t totalCPU = device->getCPUMemoryUsage();
uint64_t totalGPU = device->getGPUMemoryUsage();
```

### 5.3 Memory Snapshots

Capture a detailed breakdown by resource type:

```cpp
snap::rhi::DeviceMemorySnapshot snapshot = device->captureMemorySnapshot();

// CPU memory breakdown
std::cout << "CPU total: " << snapshot.cpu.totalSizeInBytes << " bytes ("
          << snapshot.cpu.totalResourceCount << " resources)\n";
for (const auto& group : snapshot.cpu.groups) {
    std::cout << "  " << resourceTypeToString(group.type)
              << ": " << group.totalSizeInBytes << " bytes ("
              << group.entries.size() << " resources)\n";

    // Individual resource inspection
    for (const auto& entry : group.entries) {
        if (auto resource = entry.resource.lock()) {
            std::cout << "    " << resource->getDebugLabel()
                      << ": " << entry.sizeInBytes << " bytes\n";
        }
    }
}

// GPU memory breakdown
std::cout << "GPU total: " << snapshot.gpu.totalSizeInBytes << " bytes\n";
for (const auto& group : snapshot.gpu.groups) {
    std::cout << "  " << resourceTypeToString(group.type)
              << ": " << group.totalSizeInBytes << " bytes\n";
}
```

**Snapshot structure:**

```
DeviceMemorySnapshot
├── cpu: MemoryDomainUsage
│   ├── totalSizeInBytes
│   ├── totalResourceCount
│   └── groups[]: ResourceTypeGroup
│       ├── type (Buffer, Texture, RenderPipeline, ...)
│       ├── totalSizeInBytes
│       └── entries[]: ResourceMemoryEntry
│           ├── sizeInBytes
│           └── resource (weak_ptr<DeviceChild>)
└── gpu: MemoryDomainUsage
    └── (same structure)
```

> **Tip:** Enable `SNAP_RHI_ENABLE_DEBUG_LABELS=ON` when doing memory profiling so that `resource->getDebugLabel()` returns meaningful names in the snapshot.

### 5.4 Memory Profiling Workflow

```
1. Label all resources with setDebugLabel() during creation
2. Capture snapshots at key points (scene load, level transition, etc.)
3. Compare snapshots to detect leaks or unexpected growth
4. Use per-group breakdown to identify which resource type is growing
5. Inspect individual entries via weak_ptr to find specific offenders
```

---

## 6. Platform Tools

### 6.1 macOS / iOS

| Tool | Purpose |
|------|---------|
| **Instruments (Time Profiler)** | CPU hotspots via sampling |
| **Instruments (Allocations)** | Memory allocation tracking |
| **Metal System Trace** | GPU workload and CPU/GPU interaction |
| **Xcode GPU Frame Capture** | Per-draw timing and resource inspection |

**Quick workflow:**
```bash
# 1. Build RelWithDebInfo (or use release preset)
cmake --preset macos-metal-release
cmake --build build/macos-metal-release

# 2. Run from Instruments or Xcode
# 3. Capture with Time Profiler
# 4. Analyze call tree (invert for bottom-up view)
```

### 6.2 Android

| Tool | Purpose |
|------|---------|
| **Android Studio Profiler** | CPU, memory, energy |
| **Perfetto / systrace** | System-level tracing |
| **Android GPU Inspector (AGI)** | GPU timing and counters |

**Perfetto capture:**
```bash
adb shell perfetto -o /data/misc/perfetto-traces/trace.pb -c - --time 10000 < /dev/null
adb pull /data/misc/perfetto-traces/trace.pb ./trace.pb
```

### 6.3 Windows

| Tool | Purpose |
|------|---------|
| **Visual Studio Profiler** | CPU sampling/instrumentation |
| **WPR/WPA** | System-level analysis |
| **RenderDoc** | Vulkan/OpenGL frame analysis |
| **NVIDIA Nsight Graphics** | Vulkan/OpenGL GPU profiling |

### 6.4 Linux

| Tool | Purpose |
|------|---------|
| **perf** | CPU sampling/tracing |
| **Valgrind (callgrind)** | Instruction-level profiling |
| **RenderDoc** | Vulkan/OpenGL frame analysis |
| **NVIDIA Nsight Graphics** | Vulkan GPU profiling |

---

## 7. GPU Profiling with External Tools

### 7.1 Frame Capture Analysis

| Platform | Tool | File Format |
|----------|------|-------------|
| macOS/iOS | Xcode | `.trace` |
| Cross-platform | RenderDoc | `.rdc` |
| Android | AGI | `.gfxtrace` |
| Windows | PIX | `.wpix` |

### 7.2 Key Metrics

| Metric | Interpretation |
|--------|----------------|
| **Frame time** | Total time per frame |
| **GPU time** | Time GPU spends executing (use SnapRHI timestamp queries for cross-backend measurement) |
| **Draw call count** | Potential batching opportunity |
| **State changes** | Pipeline/descriptor binding overhead |
| **Memory bandwidth** | Transfer bottlenecks |

### 7.3 Common Issues

| Issue | Indicator | Solution |
|-------|-----------|----------|
| Too many draw calls | High CPU, low GPU | Batch geometry, instancing |
| Shader complexity | Long GPU time per draw | Simplify shaders, reduce ALU |
| Memory thrashing | Allocation spikes in `captureMemorySnapshot()` | Pool allocations, reuse buffers (see [Performance Design §1.3](performance.md#13-aggressive-resource-pooling-and-reuse)) |
| Synchronization stalls | CPU waiting on GPU | Double/triple buffer, use `UnretainedResources`, async transfers |
| Excessive timestamps | Increased GPU command overhead | Reduce query density; measure at pass level, not per-draw |

---

## 8. Collecting Artifacts

### 8.1 Performance Report Checklist

- [ ] Git commit/tag
- [ ] Platform, OS, GPU, driver versions
- [ ] Build configuration (CMake preset or flags)
- [ ] Backend (Metal / Vulkan / OpenGL)
- [ ] Scene description and complexity
- [ ] SnapRHI `DeviceMemorySnapshot` (before/after)
- [ ] GPU timestamp query results (per-pass durations)
- [ ] Profile captures (Instruments, RenderDoc, etc.)
- [ ] Frame time measurements (avg, min, max, p95)

### 8.2 File Formats

| Tool | Format | Size |
|------|--------|------|
| Instruments | `.trace` | Large (can be GB) |
| RenderDoc | `.rdc` | Medium |
| Perfetto | `.pb` | Medium |
| WPR | `.etl` | Large |

### 8.3 Best Practices

```
✓ Profile on target hardware (not emulators for GPU)
✓ Use consistent power/thermal settings
✓ Run multiple captures and average
✓ Document reproduction steps
✓ Compare against baseline
✓ Disable SNAP_RHI_ENABLE_DEBUG_LABELS and SNAP_RHI_ENABLE_LOGS for timing accuracy
✓ Handle QueryPool::Result::Disjoint — discard invalid timing data
```

---

## Quick Reference

### CPU Profiling Commands

```bash
# macOS: Instruments CLI
xcrun xctrace record --template "Time Profiler" --launch -- ./myapp

# Linux: perf
perf record -g ./myapp
perf report
```

### GPU Timestamp Query — Complete Example

```cpp
// 1. Check capability
const auto& caps = device->getCapabilities();
if (!caps.queueFamilyProperties[0].isTimestampQuerySupported) {
    std::cerr << "Timestamp queries not supported on this device\n";
    return;
}

// 2. Create pool
auto queryPool = device->createQueryPool({.queryCount = 64});
auto queue = device->getCommandQueue(0, 0);
auto commandBuffer = device->createCommandBuffer({.commandQueue = queue});

// 3. Record timestamps
commandBuffer->resetQueryPool(queryPool.get(), 0, 2);

auto encoder = commandBuffer->getRenderCommandEncoder();
encoder->beginEncoding(renderPassBeginInfo);
encoder->writeTimestamp(queryPool.get(), 0, snap::rhi::TimestampLocation::Start);
encoder->bindRenderPipeline(pipeline.get());
encoder->bindVertexBuffer(0, vertexBuffer.get(), 0);
encoder->draw(vertexCount, 0, 1);
encoder->writeTimestamp(queryPool.get(), 1, snap::rhi::TimestampLocation::End);
encoder->endEncoding();

// 4. Submit and wait
auto fence = device->createFence({});
queue->submitCommands({commandBuffer.get()}, {}, {}, fence.get());
fence->waitForComplete();

// 5. Read results
std::array<std::chrono::nanoseconds, 2> results;
auto status = queryPool->getResults(0, 2, results);
if (status == snap::rhi::QueryPool::Result::Available) {
    auto duration = results[1] - results[0];
    std::cout << "GPU render pass: " << duration.count() / 1000 << " μs\n";
}
```

---

## Further Reading

- [SnapRHI Performance Design](performance.md) — Zero-allocation hot paths, pooling, retention modes
- [ADR-0002 — Timestamp Query Support](adr/0002-timestamp-query-support.md) — Design rationale and backend details
- [Instruments User Guide](https://developer.apple.com/documentation/instruments)
- [Android GPU Inspector](https://gpuinspector.dev/)
- [RenderDoc Documentation](https://renderdoc.org/docs/)
- [PIX on Windows](https://devblogs.microsoft.com/pix/)

---

*Last updated: 2026-02-20*
