# TASK
# TASK.md
## Open Engineering Platform (OEP)

Task ID: 000008

Status: Complete

---

# Current Task

Implement the Repository Audit Log per OEP-SPEC-008-REPOSITORY_AUDIT_LOG.

This delivers a strongly typed `AuditEvent`, an `AuditStore`, and automatic event recording wired directly into `ObjectStore`/`RelationshipStore` so Engineering Object and Relationship lifecycle operations cannot bypass the audit trail.

---

# Context

TASK-000007 (Repository Graph Traversal) is complete and accepted.

OEP-SPEC-008-REPOSITORY_AUDIT_LOG.md defines the seven initial event types, required fields, validation, and the `RecordEvent`/`ListEvents`/`ListEventsForObject`/`Clear` interface. It requires that Foundation — not applications — creates audit events automatically whenever objects or relationships are created, updated, or deleted.

`AuditStore` is added to `platform/repository` alongside the existing stores, consistent with prior sprints' placement decisions (no dedicated audit module exists in the frozen architecture).

Scope note: `RepositoryCreated` is implemented as a supported event type and is recordable via `AuditStore` directly, but this task does not wire it into `oep init` — OEP-SPEC-002's frozen repository structure does not specify where an audit log lives on disk within a generated repository, and deciding that convention is out of scope here. That remains open for a future task once ratified.

---

# Objectives

Complete the following tasks only.

## Objective 1

Add `AuditEvent`/`AuditEventType` to `platform/repository`: `eventId`, `timestampUtc`, `eventType`, `objectId`, `user`, `description`, plus the seven initial event types (RepositoryCreated, ObjectCreated, ObjectUpdated, ObjectDeleted, RelationshipCreated, RelationshipUpdated, RelationshipDeleted).

---

## Objective 2

Implement validation: required eventId/timestampUtc, UUIDv4 format for eventId, UUIDv4 format for objectId when non-empty.

---

## Objective 3

Implement `AuditStore`: `record_event`, `list_events`, `list_events_for_object`, `clear`. Atomic writes; an invalid event is never recorded; deterministic ordering.

---

## Objective 4

Wire an `AuditStore` into `ObjectStore` and `RelationshipStore` so every successful create/update/delete automatically records the corresponding event. Audit recording must never cause an otherwise-successful repository operation to fail.

---

## Objective 5

Add unit tests validating event recording, storage, retrieval (all events and by object), validation rejections, `clear`, and automatic recording from object/relationship lifecycle operations.

---

# Explicitly Out of Scope

Do not implement:

- Version control
- Undo/Redo
- Collaboration
- Cloud synchronization
- Wiring `RepositoryCreated` into `oep init` (no ratified on-disk location for the audit log within a generated repository yet)
- Runtime, SDK, Studios, Exchange, Networking, Authentication, Plugin system, GUI

These systems belong to future tasks.

---

# Deliverables

- `AuditEvent`/`AuditEventType` and validation
- `AuditStore`
- `ObjectStore`/`RelationshipStore` integration (automatic recording)
- Unit tests

---

# Acceptance Criteria

This task is complete only when:

- The project builds successfully.
- Audit Events are created automatically by `ObjectStore`/`RelationshipStore` operations.
- `AuditStore` exists and persists events.
- Events can be listed, and filtered by Engineering Object.
- Unit tests covering creation, storage, retrieval, validation, and repository integrity pass.

---

# Engineering Expectations

Favor readability.

Favor maintainability.

Favor simplicity.

No external JSON library dependency.

Avoid speculative implementation (no version control, no undo/redo, no CLI/generator wiring for `RepositoryCreated`).

---

# Completion Checklist

Before marking this task complete, verify:

✓ Build succeeds

✓ Documentation updated

✓ Project structure preserved

✓ Architecture unchanged

✓ Acceptance criteria satisfied

---

# After Completion

Update:

- PROJECT_STATUS.md
- CURRENT_SPRINT.md

Create the next TASK.md for the following objective.

Do not begin the next task until the current task has been reviewed and accepted.

---

# Verification Record

Built with MSVC 19.51 (Visual Studio Build Tools 18) via CMake + Ninja.

- Build: succeeded, including the new `AuditEvent`/`AuditStore` and the `ObjectStore`/`RelationshipStore` constructor change to require an `AuditStore` (all call sites, including four test files, updated accordingly)
- `oep_repository_tests`, `oep_engineering_object_tests`, `oep_relationship_tests`, `oep_graph_engine_tests`, `oep_audit_store_tests`, `oep_search_engine_tests` (CTest): 6/6 suites passed
  - Audit suite covers: record/list, list-for-object filtering, validation rejections (missing timestamp, malformed objectId), `clear`, and two integration tests verifying `ObjectStore` and `RelationshipStore` each automatically record exactly one audit event per create/update/remove
- `oep init my-workshop`: re-verified unaffected, exit code 0

Task 000008 is complete pending formal acceptance.
