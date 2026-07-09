# TASK
# TASK.md
## Open Engineering Platform (OEP)

Task ID: 000014

Status: Complete

---

# Current Task

Implement Engineering Relationship CLI Commands per OEP-SPEC-014-RELATIONSHIP_COMMANDS.

This delivers `oep relationship create|list|show|delete`, mirroring the `oep object` command pattern from TASK-000013 but backed by `RelationshipStore`, making the CLI a complete authoring interface for both Engineering Objects and Relationships.

---

# Context

TASK-000013 (Engineering Object CLI Commands) is complete and accepted.

OEP-SPEC-014-RELATIONSHIP_COMMANDS.md defines the four subcommands, required/optional creation fields, required display fields for listing/inspection, and requires that deletion automatically generate an Audit Event (already true of `RelationshipStore::remove`, wired since Sprint 008 — no new audit logic is needed). Relationship editing, batch creation, graph visualization, and automatic discovery are explicitly deferred.

Following the `object` command's convention, repository selection is via an optional `--repository <path>` flag (defaulting to the current working directory), since `show`/`delete` use their one positional slot for the relationship ID.

---

# Objectives

Complete the following tasks only.

## Objective 1

Add `RelationshipCommand` (registered as `relationship`) to `platform/cli`, dispatching to `create`/`list`/`show`/`delete` subcommands, structured consistently with `ObjectCommand`.

---

## Objective 2

`create` requires `--source`, `--target`, `--type`; accepts optional `--author`, `--description`, and `--repository`. Relationship IDs are generated automatically by `RelationshipStore::create`, which already validates that both objects exist, rejects a matching source/target, and rejects an invalid type — the CLI relies on these existing checks rather than re-validating.

---

## Objective 3

`list` displays Relationship ID, Relationship Type, Source Object ID, and Target Object ID for every relationship, sorted by Relationship ID for deterministic output.

---

## Objective 4

`show <relationship-id>` displays Relationship ID, Relationship Type, Source Object ID, Target Object ID, Author, Description, and Creation Timestamp.

---

## Objective 5

`delete <relationship-id>` removes the relationship through `RelationshipStore::remove` (which already automatically records a `RelationshipDeleted` Audit Event).

---

## Objective 6

Register `RelationshipCommand` so Help lists it consistently with existing commands.

---

## Objective 7

Update `platform/runtime/CLI_USAGE.md` with relationship command syntax, supported relationship types, example workflows, expected output, and current limitations.

---

## Objective 8

Add unit tests validating creation, listing, inspection, deletion, invalid object references, invalid relationship types, duplicate/matching source-target rejection, and Runtime integration.

---

# Explicitly Out of Scope

Do not implement:

- Relationship editing
- Batch relationship creation
- Graph visualization
- Automatic relationship discovery
- Runtime, SDK, Studios, Exchange, Networking, Authentication, Plugin system, GUI

These systems belong to future tasks.

---

# Deliverables

- `RelationshipCommand` (`create`/`list`/`show`/`delete`)
- Updated `platform/runtime/CLI_USAGE.md`
- Unit tests

---

# Acceptance Criteria

This task is complete only when:

- The project builds successfully.
- Relationships can be created, listed, shown, and deleted from the CLI.
- Relationship validation (object existence, type validity, source ≠ target) is enforced via the existing `RelationshipStore`.
- Deletion automatically generates an Audit Event.
- Help lists the `relationship` command.
- No subcommand constructs Repository services directly.
- `CLI_USAGE.md` is updated and accurate.
- Unit tests covering creation, listing, inspection, deletion, invalid object references, invalid relationship types, duplicate/matching source-target rejection, and Runtime integration pass.

---

# Engineering Expectations

Favor readability.

Favor maintainability.

Favor simplicity.

Every subcommand is a thin layer over `FoundationRuntime`/`RelationshipStore`; no business logic is duplicated in the CLI.

Avoid speculative implementation (no editing, no batch creation, no visualization).

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

- Build: succeeded, including the new `RelationshipCommand` and the shared `repository_path_option` helper factored out of `ObjectCommand`
- `oep_repository_tests`, `oep_engineering_object_tests`, `oep_relationship_tests`, `oep_graph_engine_tests`, `oep_audit_store_tests`, `oep_search_engine_tests`, `oep_repository_validator_tests`, `oep_package_manager_tests`, `oep_foundation_runtime_tests`, `oep_cli_commands_tests`, `oep_object_command_tests`, `oep_relationship_command_tests` (CTest): 12/12 suites passed
- Manual smoke test: `oep relationship create/list/show/delete` run correctly against a real generated repository with two objects, including `show` failing descriptively after deletion
- `platform/runtime/CLI_USAGE.md`: updated with the relationship command table, `--type` enum values, a full object+relationship workflow with real captured output, a validation-rejection example, and new limitations

Task 000014 is complete pending formal acceptance.
