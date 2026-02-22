# 1. Cross-API GPU Synchronization (OpenGL ⇄ Vulkan) Using External Semaphores

Date: 2025-10-9

## Status
Accepted

## Context
SnapRHI must allow OpenGL and Vulkan backends to share and sequence access to texture resources without CPU stalls or unnecessary copies. Prior approaches (implicit sync, glFinish, queue idle waits) either introduce latency, reduce parallelism, or are nondeterministic. Modern Android/Linux stacks expose external semaphore primitives (EGL native fence + Vulkan `VK_KHR_external_semaphore_fd`) enabling GPU-only ordering with transferable file descriptors.

## Decision
Adopt an abstraction `snap::rhi::Impl::Interop::ExternalSemaphore` wrapping a single-use platform fence FD.
- Producer backend (GL or Vulkan) emits an external semaphore after encoding work touching shared textures.
- Consumer backend acquires the FD, imports (Vulkan) or waits (OpenGL EGL) before executing dependent commands.
- Ownership of the FD transfers exactly once; lifetimes managed via RAII.
- Device layers expose `createExternalSemaphore()` for generating export-capable semaphores.
- Fallback path (feature flag) reverts to conservative synchronization if required extensions are missing.

## Consequences
Positive:
- Fully GPU-side synchronization; reduced frame latency.
- Clear, testable contract for future backends (Metal, DirectX).
- Eliminates global stalls; preserves queue parallelism.

Negative:
- Platform-specific complexity (FD import/export logic).
- Additional resource bookkeeping (semaphore references per submission).
- Requires robust capability detection and fallback handling.

Risks & Mitigations:
- FD leaks → RAII + tests.
- Unsupported platforms → capability gating.
- Deadlocks → unidirectional producer→consumer dependency model per submission; no circular waits.

## Alternatives Considered
1. CPU synchronization (glFinish / vkQueueWaitIdle): simplicity but high stall cost.
2. Implicit sync only: opaque hazards; difficult to validate.
3. Timeline semaphores + host signaling: broader scope, deferred pending EGL timeline maturity.

## Compliance & Extensibility
Design aligns with Vulkan external semaphore FD spec and EGL native fence extension. Abstraction layer isolates platform code enabling future addition of timeline semaphores or Metal shared events.
