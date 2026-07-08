# TASK
# TASK.md
## Open Engineering Platform (OEP)

Task ID: 000002

Status: Complete

---

# Current Task

Implement the Foundation Generator inside the OEP CLI.

This delivers `oep init <repository-name>`, generating a Standard Repository exactly as defined by OEP-SPEC-002-FOUNDATION_REPOSITORY.

---

# Context

TASK-000001 (OEP CLI command architecture) is complete and accepted.

OEP-SPEC-002-FOUNDATION_REPOSITORY.md defines the canonical repository structure the generator must produce.

---

# Objectives

Complete the following tasks only.

## Objective 1

Implement the `init` command using the existing `oep::cli::Command` extension point.

---

## Objective 2

Implement a Foundation Repository generator that creates, at `<repository-name>`:

- `repository/`, `workspace/`, `packages/`, `cache/`, `logs/`, `exports/`, `settings/`
- `README.md`, `.gitignore`, `repository.json`, `workspace.json`

Per OEP-SPEC-002 sections 5–7.

---

## Objective 3

Generation must be deterministic in structure (identical directories/files every run) with a globally unique repository ID per generated repository, and must fail safely (no partial repository) if the destination already exists and is non-empty.

---

## Objective 4

Verify the build and exercise `oep init`.

Confirm:

- Project compiles
- `oep init <name>` produces the exact structure required by OEP-SPEC-002
- Re-running against an existing non-empty directory fails without modifying it

---

# Explicitly Out of Scope

Do not implement:

- Runtime
- SDK
- Studios
- Repository Engine (reading/validating Engineering Objects)
- Exchange
- Networking
- Authentication
- Plugin system
- GUI
- Enterprise/Educational/Embedded repository types

These systems belong to future tasks.

---

# Deliverables

- `init` command
- Foundation Repository generator
- Updated documentation

---

# Acceptance Criteria

This task is complete only when:

- The project builds successfully.
- `oep init <repository-name>` creates the repository exactly as specified in OEP-SPEC-002.
- No undocumented files or directories are generated.
- The architecture remains modular.

---

# Engineering Expectations

Favor readability.

Favor maintainability.

Favor simplicity.

Leave extension points for future repository types.

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

- Build: succeeded
- `oep init my-workshop`: generated exactly the structure required by OEP-SPEC-002 (7 directories, 4 root files), valid UUIDv4 `repositoryId`, exit code 0
- Re-running `oep init my-workshop` against the now non-empty directory: failed safely, exit code 1, no files modified

Task 000002 is complete pending formal acceptance.
