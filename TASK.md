# TASK
# TASK.md
## Open Engineering Platform (OEP)

Task ID: 000003

Status: Complete

---

# Current Task

Implement the Repository Metadata System per OEP-SPEC-003-REPOSITORY_METADATA.

This delivers a strongly typed `repository.json` metadata model, with a loader, a writer, and validation, used by the Foundation Generator and available to future Foundation components.

---

# Context

TASK-000002 (Foundation Generator / `oep init`) is complete and accepted.

OEP-SPEC-003-REPOSITORY_METADATA.md defines the metadata schema, identity rules, and load/save/validation requirements. Sprint 003 focuses exclusively on `repository.json`.

---

# Objectives

Complete the following tasks only.

## Objective 1

Add a new `platform/repository` module (library) exposing a strongly typed `RepositoryMetadata` object matching the OEP-SPEC-003 schema.

---

## Objective 2

Implement a metadata loader that parses `repository.json`, validates required fields, UUID format, version format, and reports descriptive errors on malformed input without modifying anything.

---

## Objective 3

Implement a metadata writer that saves `RepositoryMetadata`, updates `lastModifiedUtc`, and writes atomically (never leaving a partially-written file).

---

## Objective 4

Update the Foundation Generator (`oep init`) to populate `repository.json` through this new metadata writer instead of hand-written JSON.

---

## Objective 5

Add unit tests validating the loader (valid file, invalid JSON, missing required fields, bad UUID) and the writer (round-trip, timestamp update).

---

# Explicitly Out of Scope

Do not implement:

- Registry synchronization
- Authentication
- Cloud services
- workspace.json schema changes
- Runtime, SDK, Studios, Exchange, Networking, Plugin system, GUI

These systems belong to future tasks.

---

# Deliverables

- `platform/repository` library (metadata model, loader, writer, validation)
- Updated Foundation Generator
- Unit tests for loader and writer

---

# Acceptance Criteria

This task is complete only when:

- The project builds successfully.
- Repository metadata loads successfully from a well-formed `repository.json`.
- Invalid metadata (malformed JSON, missing fields, bad UUID) is detected and reported without modifying files.
- Metadata can be written safely (atomic write, updated timestamp).
- `oep init` produces `repository.json` via the new metadata writer.
- Unit tests pass.

---

# Engineering Expectations

Favor readability.

Favor maintainability.

Favor simplicity.

No external JSON library dependency — implement the minimal parser/serializer this schema requires.

Avoid speculative implementation.

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

- Build: succeeded (`platform/repository`, `platform/cli`, `tests/repository`)
- `oep_repository_tests` (CTest): 6/6 cases passed — valid load, invalid JSON rejected, missing required fields rejected, malformed UUID rejected, save/round-trip refreshes `lastModifiedUtc`, invalid metadata rejected on save with no file written
- `oep init my-workshop`: `repository.json` generated via the new metadata writer, matches OEP-SPEC-003 schema exactly, no leftover `.tmp` file, exit code 0

Task 000003 is complete pending formal acceptance.
