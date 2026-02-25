## Summary

<!-- One-paragraph description of **what** this PR does and **why**. -->


## Motivation

<!-- Link the issue/feature request, or explain the problem being solved. -->

Closes #<!-- issue number -->

## What changed

<!-- Describe the overall approach. Call out non-obvious design decisions. -->


## Backend impact

<!-- For each backend, explain what changed or write "No changes". -->

| Backend | Changes |
|---------|---------|
| **Metal** | |
| **Vulkan** | |
| **OpenGL / ES** | |
| **Noop** | No changes |

## Breaking changes

<!-- List any public API or behavioral changes that affect consumers. Write "None" if not applicable. -->

None

## Test plan

- [ ] Builds on all affected platforms (`cmake --preset …`)
- [ ] Verified in Demo App (triangle-demo-app)
- [ ] Validation enabled (`-DSNAP_RHI_ENABLE_VALIDATION=ON`) — no new warnings
- [ ] Existing tests pass
- [ ] <!-- Add scenario-specific verification steps -->

## Performance considerations

<!-- Will this affect hot paths? If yes, describe the expected impact and any benchmarks. Write "N/A" for non-perf-sensitive changes. -->

N/A

## Screenshots / frame captures

<!-- For visual changes: attach screenshots, frame captures, or RenderDoc/Xcode GPU snapshots. Delete this section if not applicable. -->

## Checklist

- [ ] Code follows the project's coding standards and passes `clang-format`
- [ ] Self-reviewed the diff for correctness and clarity
- [ ] Updated documentation (if public API changed)
- [ ] Added/updated tests (if applicable)
