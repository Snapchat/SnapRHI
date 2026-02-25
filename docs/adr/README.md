# Architecture Decision Records

This directory contains Architecture Decision Records (ADRs) documenting significant technical decisions made during SnapRHI development.

## Index

| ADR | Title | Status | Date |
|-----|-------|--------|------|
| [ADR-0001](0001-opengl-vulkan-sync.md) | Cross-API GPU Synchronization (OpenGL ⇄ Vulkan) | Accepted | 2025-10-09 |
| [ADR-0002](0002-timestamp-query-support.md) | Timestamp Query Support | Accepted | 2025-10-30 |
| [ADR-0003](0003-textureinterop-read-write-api.md) | TextureInterop Read/Write API | Accepted | 2025-11-05 |
| [ADR-0004](0004-explicit-memory-management-buffer-mapping.md) | Explicit Memory Management & Buffer Mapping | Proposed | 2026-01-28 |

## About ADRs

ADRs are short documents that capture important architectural decisions along with their context and consequences. They help:

- **New team members** understand why things are the way they are
- **Future maintainers** make informed decisions about changes
- **Reviewers** understand the reasoning behind designs

## Creating New ADRs

1. Copy the template below
2. Name the file `NNNN-short-title.md` (next sequential number)
3. Fill in all sections
4. Submit for review with your implementation PR

### Template

```markdown
# N. Title

Date: YYYY-MM-DD
Author: Name
Status: Proposed | Accepted | Deprecated | Superseded

## Context
What is the issue that we're seeing that is motivating this decision?

## Decision
What is the change that we're proposing and/or doing?

## Consequences
What becomes easier or more difficult as a result of this change?

## Alternatives Considered
What other options were evaluated?
```

---

*See [about.md](../about.md) for API documentation and [resource-management.md](../resource-management.md) for usage contracts.*
