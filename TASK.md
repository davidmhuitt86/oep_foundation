# TASK
# TASK.md
## Open Engineering Platform (OEP)

Task ID: 000004

Status: Complete

---

# Current Task

Implement the Engineering Object Model per OEP-SPEC-004-ENGINEERING_OBJECT_MODEL.

This delivers a strongly typed `EngineeringObject`, validation, serialization, and a Create/Read/Update/Delete store, so that future subsystems operate on Engineering Objects rather than files directly.

---

# Context

TASK-000003 (Repository Metadata System) is complete and accepted.

OEP-SPEC-004-ENGINEERING_OBJECT_MODEL.md defines object identity, metadata, classification, lifecycle, and validation requirements. File formats and object relationships are explicitly out of scope for this spec.

---

# Objectives

Complete the following tasks only.

## Objective 1

Add `EngineeringObject` to `platform/repository`: `objectId`, `objectType`, `name`, `description`, `createdUtc`, `lastModifiedUtc`, `version`, `author`, `tags`, plus the six initial object types (Document, Diagram, Component, Procedure, Project, Image).

---

## Objective 2

Implement validation: required fields, UUIDv4 `objectId`, valid `objectType`, version format.

---

## Objective 3

Implement serialization/deserialization (to/from JSON) preserving all metadata.

---

## Objective 4

Implement a Create/Read/Update/Delete object store: creation generates the object ID and timestamps; updates preserve `objectId`/`objectType`/`createdUtc` and refresh `lastModifiedUtc`; all writes are atomic; validation failures never modify repository contents.

---

## Objective 5

Add unit tests validating object creation, loading, saving (update), and validation failures.

---

# Explicitly Out of Scope

Do not implement:

- Object relationships (future specification)
- File format standardization
- Registry synchronization
- User interface behavior
- Soft-delete (noted in spec as a future revision)
- Runtime, SDK, Studios, Exchange, Networking, Authentication, Plugin system, GUI

These systems belong to future tasks.

---

# Deliverables

- `EngineeringObject` model and validation
- Object serialization/deserialization
- CRUD object store
- Unit tests

---

# Acceptance Criteria

This task is complete only when:

- The project builds successfully.
- `EngineeringObject` exists with strongly typed metadata.
- UUID identity is enforced.
- Object validation succeeds/fails correctly.
- Serialization and deserialization round-trip successfully.
- Unit tests covering creation, loading, saving, and validation pass.

---

# Engineering Expectations

Favor readability.

Favor maintainability.

Favor simplicity.

No external JSON library dependency.

Avoid speculative implementation (no relationships, no soft-delete yet).

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

- Build: succeeded, including a UUID/timestamp consolidation refactor (removed a duplicate UUID generator that had existed in `platform/cli`)
- `oep_repository_tests` and `oep_engineering_object_tests` (CTest): 2/2 suites passed, 14 assertions total
  - Engineering Object suite covers: create assigns objectId/timestamps, load round-trip, load fails for a missing object, update preserves objectId/objectType/createdUtc while refreshing lastModifiedUtc, remove deletes the object, and validation rejects a missing name / malformed objectId
- `oep init my-workshop`: still produces a correct `repository.json` after the refactor, exit code 0

Task 000004 is complete pending formal acceptance.
