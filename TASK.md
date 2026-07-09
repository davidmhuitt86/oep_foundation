# TASK
# TASK.md
## Open Engineering Platform (OEP)

Task ID: 000020

Status: Complete

---

# Current Task

Implement Batch Repository Operations per OEP-SPEC-020-BATCH_OPERATIONS (Work Package 010, second of two tasks; follows TASK-000019/Repository Templates immediately, per instruction to complete both before stopping).

This delivers `oep batch create|delete|validate`, allowing many Engineering Objects and Relationships to be created or deleted from a single deterministic JSON input file, fully validated before any execution begins.

---

# Context

TASK-000019 (Repository Templates) is complete, built, and tested.

Batch logic lives in `platform/repository` (a new `BatchProcessor`), not `platform/exchange` — bulk object/relationship CRUD against an already-open repository is Repository-domain business logic, unlike Export/Import/Templates, which package/reconstruct whole repositories. `BatchProcessor` reuses the already-public `oep::repository::json` parser; no new JSON infrastructure is introduced.

Batch input format (documented in `CLI_USAGE.md`):

```json
{
  "objects": [
    { "ref": "coil", "type": "Component", "name": "Ignition Coil", "author": "Jane", "tags": ["electrical"] }
  ],
  "relationships": [
    { "source": "manual", "target": "coil", "type": "Documents", "description": "..." }
  ]
}
```

`ref` is a batch-local alias (not a real object ID) that a relationship's `source`/`target` may reference for an object created earlier in the *same* batch; a `source`/`target` that doesn't match any `ref` in the batch is treated as an existing object's real ID and checked against the open repository. Batch delete takes a simpler `{"objectIds": [...], "relationshipIds": [...]}` shape. `batch validate` checks a create-format batch file without executing it.

Every batch is fully parsed and validated (duplicate `ref`s, unresolvable `source`/`target`, invalid `type` values, missing required fields) before a single object or relationship is created — matching the same validate-before-write discipline `RepositoryValidator`/Import/Templates already use.

---

# Objectives

Complete the following tasks only.

## Objective 1

Add `BatchProcessor` to `platform/repository`: parses and validates a batch-create request and a batch-delete request, and executes each only after full validation succeeds.

---

## Objective 2

Extend `FoundationRuntime` with `execute_batch_create`, `execute_batch_delete`, and `validate_batch_create` (all require an open repository).

---

## Objective 3

Add `oep batch create|delete|validate` to `platform/cli`, each taking an input file path and `--repository <path>` (default cwd).

---

## Objective 4

Register `BatchCommand` so Help lists it consistently with existing commands.

---

## Objective 5

Update `platform/runtime/CLI_USAGE.md` with the batch input format (both create and delete shapes), command syntax, and example workflows.

---

## Objective 6

Add unit tests validating: successful batch creation (including in-batch relationship refs), successful batch deletion, batch validation (both passing and failing), duplicate identifiers/refs, invalid object references, invalid relationship definitions, and malformed batch files.

---

# Explicitly Out of Scope

Do not implement:

- Transaction rollback (validation-before-execution is the integrity guarantee; a mid-execution filesystem failure is not rolled back, consistent with every other store in this codebase)
- Concurrent execution
- Distributed processing
- Cloud synchronization
- Runtime, SDK, Studios, Networking, Authentication, Plugin system, GUI

These systems belong to future tasks.

---

# Deliverables

- `BatchProcessor` (`platform/repository`)
- `FoundationRuntime::execute_batch_create` / `execute_batch_delete` / `validate_batch_create`
- `BatchCommand`
- Updated `platform/runtime/CLI_USAGE.md`
- Unit tests

---

# Acceptance Criteria

This task is complete only when:

- The project builds successfully.
- Batch creation, deletion, and validation all succeed for well-formed input.
- Validation runs fully before any execution; a validation failure creates/deletes nothing.
- Runtime integration is preserved — the CLI never manipulates repository contents directly.
- `CLI_USAGE.md` is updated.
- Unit tests covering successful execution, validation failures, duplicate identifiers, invalid relationships, and malformed batch files pass.
- A full clean rebuild succeeds and the complete regression suite passes.

---

# Engineering Expectations

Favor readability.

Favor maintainability.

Favor simplicity.

No external JSON library dependency; reuse `oep::repository::json`.

Avoid speculative implementation (no rollback, no concurrency, no distributed processing).

---

# Completion Checklist

Before marking this task complete, verify:

✓ Build succeeds

✓ Full regression suite passes

✓ Documentation updated (`CLI_USAGE.md`)

✓ Project structure preserved

✓ Architecture unchanged

✓ Acceptance criteria satisfied

---

# After Completion

Update:

- PROJECT_STATUS.md
- CURRENT_SPRINT.md

Stop and await formal review of Work Package 010 (TASK-000019 + TASK-000020), per instructions.

---

# Verification Record

**Full clean rebuild:** `rm -rf build` followed by `cmake -S . -B build -G Ninja` and `cmake --build build` with MSVC 19.51 (Visual Studio Build Tools 18) — 95/95 build steps succeeded, producing every library (`oep_repository`, `oep_packages`, `oep_search`, `oep_validation`, `oep_exchange`, `oep_runtime`, `oep_cli_core`), the `oep` executable, and all 22 test executables.

**Regression suite:** `ctest --output-on-failure` — 22/22 test suites passed (100%), including the two new suites added by this task (`oep_batch_processor_tests`, `oep_batch_command_tests`) and the two added by TASK-000019 (`oep_repository_template_tests`, `oep_template_command_tests`), alongside all eighteen pre-existing suites.

**Manual smoke tests:** `oep init`, plus a full `oep batch create` → `oep batch validate` (both passing and finding a deliberate duplicate ref) → `oep batch delete` session against a real generated repository, confirming in-batch `ref` resolution, deterministic multi-finding validation reporting, and that a failed validation creates nothing.

**Architectural observations and decisions:**

- `BatchProcessor` was placed in `platform/repository`, not `platform/exchange`, because it operates on an *already-open* repository's objects/relationships — the same domain `GraphEngine` and `RepositoryValidator` already occupy — rather than packaging or reconstructing a *whole* repository (Export/Import/Templates' concern). This keeps `platform/exchange` scoped to whole-repository transfer formats.
- The `ref` aliasing mechanism (batch-local, resolved by `validate_batch_create_request`/`execute_batch_create` via a `known_refs` set with fallback to `ObjectStore::load`) was necessary because new objects don't have real IDs until they're created — without it, a batch could never create a new object and a relationship to it in one atomic request.
- Both `BatchCreateResult`/`BatchDeleteResult` "all or nothing" semantics required validating fully (all refs, all IDs) before writing anything, consistent with the validate-then-execute discipline already established by `RepositoryValidator`, Import, and Templates.
- `TemplateManifest` JSON marshaling was added to the existing `archive_codec.hpp` rather than a new codec file, since a template is structurally "an archive plus a manifest" and already needed every marshaling function the exporter/importer had.
- The `split_csv` shared-refactoring extraction (moving `object_command`'s local `split_tags` into `repository_path_option.hpp`) was completed under the work package's pre-authorization for shared refactoring between the two tasks; no other public interface or architectural boundary was touched.
