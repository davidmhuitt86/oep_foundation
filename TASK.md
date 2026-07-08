# TASK
# TASK.md
## Open Engineering Platform (OEP)

Task ID: 000013

Status: Complete

---

# Current Task

Implement Engineering Object CLI Commands per OEP-SPEC-013-ENGINEERING_OBJECT_COMMANDS.

This delivers `oep object create|list|show|delete`, making the CLI the first engineering-authoring client for OEP, backed exclusively by Foundation Runtime and the existing `ObjectStore`.

---

# Context

TASK-000012 (CLI Command Framework Expansion) is complete and accepted.

OEP-SPEC-013-ENGINEERING_OBJECT_COMMANDS.md defines the four subcommands, required fields for creation, required display fields for listing/inspection, and requires that deletion automatically generate an Audit Event (already true of `ObjectStore::remove`, wired since Sprint 008 — no new audit logic is needed here). Object editing, binary asset import, relationship management, and diagram editing are explicitly deferred.

Unlike `validate`/`packages`/`status` (which take an optional positional repository path), the spec's literal syntax for `object show <object-id>`/`object delete <object-id>` already uses the one positional slot for the object ID. Repository selection for all four `object` subcommands is via an optional `--repository <path>` flag, defaulting to the current working directory — consistent in spirit with the existing commands' default-to-cwd behavior, expressed as a flag instead of a positional argument since the object ID already occupies that position for `show`/`delete`.

---

# Objectives

Complete the following tasks only.

## Objective 1

Add `ObjectCommand` (registered as `object`) to `platform/cli`, dispatching to `create`/`list`/`show`/`delete` subcommands.

---

## Objective 2

`create` accepts `--type`, `--name`, `--description`, `--author`, `--tags` (comma-separated) and an optional `--repository`; `--type` and `--name` are required. Object IDs are generated automatically by `ObjectStore::create`.

---

## Objective 3

`list` displays Object ID, Object Type, Name, and Version for every object, sorted by Object ID for deterministic output.

---

## Objective 4

`show <object-id>` displays every field: Object ID, Object Type, Name, Description, Author, Tags, Version, Creation Timestamp, Last Modified Timestamp.

---

## Objective 5

`delete <object-id>` removes the object through `ObjectStore::remove` (which already automatically records an `ObjectDeleted` Audit Event).

---

## Objective 6

Register `ObjectCommand` so Help lists it (and its subcommands' existence is discoverable via its own usage/description) consistently with existing commands.

---

## Objective 7

Update `platform/runtime/CLI_USAGE.md` with object command syntax, example workflows, expected output, and current limitations.

---

## Objective 8

Add unit tests validating creation, listing, inspection, deletion, invalid/nonexistent object IDs, and that all four subcommands operate exclusively through Foundation Runtime.

---

# Explicitly Out of Scope

Do not implement:

- Object editing (updating an existing object's fields via CLI)
- Binary asset import
- Relationship management commands
- Diagram editing
- Filtering/sorting options for `list` beyond deterministic ID order
- Soft deletion
- Runtime, SDK, Studios, Exchange, Networking, Authentication, Plugin system, GUI

These systems belong to future tasks.

---

# Deliverables

- `ObjectCommand` (`create`/`list`/`show`/`delete`)
- Updated `platform/runtime/CLI_USAGE.md`
- Unit tests

---

# Acceptance Criteria

This task is complete only when:

- The project builds successfully.
- Engineering Objects can be created, listed, shown, and deleted from the CLI.
- Deletion automatically generates an Audit Event.
- Help lists the `object` command.
- No `object` subcommand constructs Repository services directly — all go through `FoundationRuntime`.
- `CLI_USAGE.md` is updated and accurate.
- Unit tests covering creation, listing, inspection, deletion, invalid object IDs, and Runtime integration pass.

---

# Engineering Expectations

Favor readability.

Favor maintainability.

Favor simplicity.

Every subcommand is a thin layer over `FoundationRuntime`/`ObjectStore`; no business logic is duplicated in the CLI.

Avoid speculative implementation (no editing, no binary import, no relationship commands).

---

# Completion Checklist

Before marking this task complete, verify:

✓ Build succeeds

✓ Documentation updated (`CLI_USAGE.md`)

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

- Build: succeeded, including the new `ObjectCommand` and its wiring into `oep_cli_core`
- `oep_repository_tests`, `oep_engineering_object_tests`, `oep_relationship_tests`, `oep_graph_engine_tests`, `oep_audit_store_tests`, `oep_search_engine_tests`, `oep_repository_validator_tests`, `oep_package_manager_tests`, `oep_foundation_runtime_tests`, `oep_cli_commands_tests`, `oep_object_command_tests` (CTest): 11/11 suites passed
- Manual smoke test: `oep object create/list/show/delete` run correctly against a real generated repository, including `show` failing descriptively after the object is deleted
- `platform/runtime/CLI_USAGE.md`: updated with the object command table, `--type` enum values, a full example workflow with real captured output, and new limitations

Task 000013 is complete pending formal acceptance.
