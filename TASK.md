# TASK
# TASK.md
## Open Engineering Platform (OEP)

Task ID: 000018

Status: Complete

---

# Current Task

Implement Repository Import per OEP-SPEC-018-REPOSITORY_IMPORT (Work Package 009, second of two tasks; follows TASK-000017/Repository Export immediately, per instruction to complete both before stopping).

This delivers `oep import <archive-file>`, reconstructing a repository from a previously exported archive through Foundation Runtime.

---

# Context

TASK-000017 (Repository Export) is complete, built, and tested.

Import is implemented in the same `platform/exchange` module as Export, using the same archive format/codec. It required one further addition to `platform/repository`: `ObjectStore`/`RelationshipStore`/`AuditStore` gained `restore()` methods (additive-only, alongside the existing `create`/`update`/`remove`/`list_all`) so import can write back records with their exact original identifiers and timestamps — `create()` always regenerates `lastModifiedUtc` and would corrupt round-trip fidelity if reused for import.

`FoundationRuntime::import_repository` does not require (or use) a currently open repository, since it targets a destination to create, not one to open — but it is still exposed on `FoundationRuntime` rather than called directly by the CLI, so import continues to execute "entirely through Foundation Runtime" as the spec requires.

---

# Objectives

Complete the following tasks only.

## Objective 1

Add `oep::exchange::import_repository` to `platform/exchange`: reads and validates the archive (JSON well-formedness, export manifest fields, supported export version, repository metadata validity, and that every object/relationship/audit-event/package record parses) before writing anything to `destination`.

---

## Objective 2

Reject an existing non-empty `destination` unless `overwrite` is set, in which case existing contents are removed before reconstruction.

---

## Objective 3

Restore objects, then relationships, then audit events (order matters: relationships validate that their endpoint objects already exist), then packages, using the new `restore()` operations so identifiers and timestamps are preserved exactly as exported.

---

## Objective 4

Extend `FoundationRuntime` with `import_repository(archive_file, destination, overwrite)`, delegating entirely to `oep::exchange::import_repository`.

---

## Objective 5

Add `oep import <archive-file>` to `platform/cli`: `--destination <path>` (default cwd) and `--overwrite` (boolean flag, via the shared `extract_flag` helper from TASK-000017).

---

## Objective 6

Register `ImportCommand` so Help lists it consistently with existing commands.

---

## Objective 7

Update `platform/runtime/CLI_USAGE.md` with import command syntax, overwrite behavior, and expected output, plus a combined export→import round-trip example.

---

## Objective 8

Add unit tests validating: successful import, overwrite behavior (rejecting without it, succeeding with it), invalid archives, corrupted manifests, unsupported export version, and package restoration.

---

# Explicitly Out of Scope

Do not implement:

- Merge operations
- Conflict resolution
- Incremental import
- Cloud synchronization
- Runtime, SDK, Studios, Networking, Authentication, Plugin system, GUI

These systems belong to future tasks.

---

# Deliverables

- `oep::exchange::import_repository`
- `FoundationRuntime::import_repository`
- `ImportCommand`
- Updated `platform/runtime/CLI_USAGE.md`
- Unit tests

---

# Acceptance Criteria

This task is complete only when:

- The project builds successfully.
- Repository import succeeds and faithfully reconstructs an exported repository (identifiers/timestamps preserved).
- Invalid archives, corrupted manifests, and unsupported export versions are all rejected with descriptive errors.
- An existing destination is protected unless `--overwrite` is given.
- Runtime integration is preserved — the CLI never reconstructs repository contents directly.
- `CLI_USAGE.md` is updated.
- Unit tests covering successful import, overwrite behavior, invalid archives, corrupted manifests, version mismatch, and package restoration pass.
- A full rebuild succeeds and the complete regression suite passes.

---

# Engineering Expectations

Favor readability.

Favor maintainability.

Favor simplicity.

No merge/conflict-resolution logic; import always reconstructs a whole repository from a whole archive.

Avoid speculative implementation (no incremental import, no cloud sync).

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

Stop and await formal review of Work Package 009 (TASK-000017 + TASK-000018), per instructions.

---

# Verification Record

Full clean rebuild (`rm -rf build`, reconfigure, rebuild) with MSVC 19.51 (Visual Studio Build Tools 18) via CMake + Ninja: succeeded, 82/82 build steps.

Complete regression suite (`ctest --output-on-failure`): 18/18 suites passed —
`oep_repository_tests`, `oep_engineering_object_tests`, `oep_relationship_tests`, `oep_graph_engine_tests`, `oep_audit_store_tests`, `oep_search_engine_tests`, `oep_repository_validator_tests`, `oep_package_manager_tests`, `oep_repository_exporter_tests`, `oep_repository_importer_tests`, `oep_foundation_runtime_tests`, `oep_cli_commands_tests`, `oep_object_command_tests`, `oep_relationship_command_tests`, `oep_search_command_tests`, `oep_graph_command_tests`, `oep_export_command_tests`, `oep_import_command_tests`.

New test suites for this work package:
- `oep_repository_exporter_tests` (4 cases): export with content, empty-repository export, invalid output path, package inclusion on/off
- `oep_repository_importer_tests` (7 cases): successful import preserving identity, overwrite rejection then acceptance, invalid JSON, missing archive file, corrupted manifest, unsupported export version, package restoration
- `oep_export_command_tests` (4 cases): archive produced with manifest, empty-repository export, missing output file, missing repository
- `oep_import_command_tests` (5 cases): full round-trip through the real CLI commands, missing archive argument, overwrite rejection/acceptance, invalid archive, missing archive file

Manual smoke tests against real `oep init`-generated repositories: full export→import round trip (exact object ID, author, tags, and both timestamps preserved across the round trip), `--include-packages` behavior, overwrite protection (rejected without `--overwrite`, succeeds with it), and invalid-archive/missing-repository error paths — all produced descriptive, non-stack-trace errors with correct exit codes.

Documentation: `platform/runtime/CLI_USAGE.md` (export/import command tables, archive format explanation, full round-trip example with real captured output, overwrite/invalid-archive examples, new limitations), `platform/runtime/README.md`, `platform/cli/README.md`, and `platform/exchange/README.md` (Placeholder → Foundation) all updated.

## Architectural observations and implementation decisions

- **Archive format**: chosen as a single JSON document rather than a zip/tar container, to avoid introducing a compression/archive-library dependency (per the constitution's dependency-cost test). A JSON file still satisfies "a single archive containing" the required sections and is trivially deterministic to produce.
- **New `ObjectStore`/`RelationshipStore`/`AuditStore::restore()` methods**: additive-only; no existing signature or behavior changed. This was a genuine requirement, not a convenience — `create()` unconditionally regenerates `lastModifiedUtc`, which would silently corrupt round-trip fidelity if reused for import. `restore()` writes exactly what it's given and records no audit event, since restoring history is not creating new history.
- **`FoundationRuntime::import_repository` does not require an open repository.** It targets a destination to create, not one to open — but is still exposed on the Runtime (not called directly from Exchange by the CLI) so that "import executes entirely through Foundation Runtime" remains true structurally, matching how every other repository-touching command already works.
- **Generalized `--repository` flag extraction** (`extract_path_option`/`extract_flag`) so `--include-packages`, `--destination`, and `--overwrite` share one primitive instead of each command reimplementing flag parsing; `extract_repository_path`'s existing signature and behavior are unchanged.
- **Package restoration scope**: import restores only each package's `package.json` manifest, not arbitrary other files a package directory might contain — consistent with `PackageManager` itself only ever inspecting `package.json` (Sprint 010). Documented explicitly in `CLI_USAGE.md` rather than silently doing a partial restore.
- **No rollback on mid-restore filesystem failure**: archive content is fully parsed and validated *before* any filesystem write begins, so a corrupt/invalid archive never touches `destination`. A genuine filesystem failure partway through restoring already-validated records (disk full, permissions) can still leave a partially-populated destination — consistent with how every other store in this codebase behaves (atomic per-file, not atomic across a whole batch) and not something this task introduces uniquely.
