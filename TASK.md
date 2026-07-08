# TASK
# TASK.md
## Open Engineering Platform (OEP)

Task ID: 000009

Status: Complete

---

# Current Task

Implement Repository Integrity Validation per OEP-SPEC-009-REPOSITORY_VALIDATION.

This delivers a `RepositoryValidator` in the `platform/validation` module (previously scaffolded and empty), producing a deterministic Validation Report over a repository's metadata, Engineering Objects, and Relationships.

---

# Context

TASK-000008 (Repository Audit Log) is complete and accepted.

OEP-SPEC-009-REPOSITORY_VALIDATION.md defines the `ValidateRepository`/`ValidateObjects`/`ValidateRelationships`/`ValidateMetadata`/`GenerateValidationReport` interface, the ten integrity checks Sprint 009 must verify, and the Severity/Category/Message/ObjectId shape of each finding. Repair, automatic correction, and any cloud/registry validation are explicitly out of scope. Validation is strictly read-only and must not modify repository contents or create temporary files.

`platform/validation` already exists as a scaffolded, empty module per the frozen architecture (OEP-ARCH-001) — this task specifies and fills it in for the first time, the same pattern used for `platform/search` in Sprint 006.

---

# Objectives

Complete the following tasks only.

## Objective 1

Add finding/report types to `platform/validation`: `Severity` (Information/Warning/Error), a finding with severity/category/message/optional object ID, and a `ValidationReport` (status, error/warning/information counts, findings).

---

## Objective 2

Implement `RepositoryValidator` with `validate_repository`, `validate_objects`, `validate_relationships`, `validate_metadata`, operating over a `RepositoryMetadata`, an `ObjectStore`, a `RelationshipStore`, and an `AuditStore`.

---

## Objective 3

Implement the ten integrity checks: metadata exists/is valid, repository ID is valid, object IDs are unique, relationship IDs are unique, relationship endpoints exist, audit events reference valid objects when applicable, no duplicate UUIDs across objects/relationships, no invalid object types, no invalid relationship types.

---

## Objective 4

Validation must continue after encountering an error (collect all findings rather than stopping at the first), never modify repository contents, and never create temporary files. Reports must be deterministic for a given repository state.

---

## Objective 5

Add unit tests validating: a healthy repository produces no errors, each of the ten integrity rules is individually detected when violated, validation continues past an error, and report determinism.

---

# Explicitly Out of Scope

Do not implement:

- Repository repair
- Automatic correction
- Cloud validation
- Registry validation
- Runtime, SDK, Studios, Exchange, Networking, Authentication, Plugin system, GUI

These systems belong to future tasks.

---

# Deliverables

- `platform/validation` module (`RepositoryValidator`, finding/report types)
- Unit tests

---

# Acceptance Criteria

This task is complete only when:

- The project builds successfully.
- Repository validation succeeds (zero errors) on a healthy repository.
- Corrupted/inconsistent repositories are detected across all ten checks.
- Validation reports are generated and deterministic.
- Validation continues after encountering a failure.
- Unit tests covering every integrity rule pass.

---

# Engineering Expectations

Favor readability.

Favor maintainability.

Favor simplicity.

Validation must be read-only: no writes, no temporary files, no repair logic.

Avoid speculative implementation (no auto-correction, no registry/cloud checks).

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

- Build: succeeded, including the new `platform/validation` module and the `invalid_entries` addition to `ObjectStore`/`RelationshipStore`'s `list_all`
- `oep_repository_tests`, `oep_engineering_object_tests`, `oep_relationship_tests`, `oep_graph_engine_tests`, `oep_audit_store_tests`, `oep_search_engine_tests`, `oep_repository_validator_tests` (CTest): 7/7 suites passed
  - Validator suite (12 cases) covers all ten integrity checks individually: metadata missing, metadata invalid JSON, invalid repository ID, duplicate object ID, duplicate relationship ID, missing relationship endpoint, invalid object type, invalid relationship type, UUID reused across an object and a relationship, plus a healthy-repository baseline, continuation past multiple simultaneous errors, and report determinism
- `oep init my-workshop`: re-verified unaffected, exit code 0

Task 000009 is complete pending formal acceptance.
