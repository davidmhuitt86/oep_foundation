# TASK
# TASK.md
## Open Engineering Platform (OEP)

Task ID: 000015

Status: Complete

---

# Current Task

Implement Repository Search CLI Commands per OEP-SPEC-015_SEARCH_COMMANDS.

This delivers `oep search <query>`, `oep search objects <query>`, and `oep search relationships <query>`, exposing the existing `SearchEngine` through Foundation Runtime, with post-search filtering by `--type`/`--author`/`--tag`.

---

# Context

TASK-000014 (Engineering Relationship CLI Commands) is complete and accepted.

OEP-SPEC-015_SEARCH_COMMANDS.md defines the three command forms, required object/relationship result fields, the three supported filters (applied after Search Engine execution, never reordering results), and requires the CLI never instantiate `SearchEngine` directly or implement independent ranking. Semantic search, AI-assisted search, regex search, saved searches, and advanced query languages are explicitly deferred.

`--tag` has no effect on relationship search, since `Relationship` has no tags field (only `EngineeringObject` does) — this is documented rather than silently mis-filtering or erroring.

---

# Objectives

Complete the following tasks only.

## Objective 1

Add `SearchCommand` (registered as `search`) to `platform/cli`. `oep search <query>` searches both objects and relationships; `oep search objects <query>` / `oep search relationships <query>` search just one.

---

## Objective 2

Support `--type`, `--author`, `--tag` filters (plus the existing `--repository`), applied by the CLI after `SearchEngine` returns results — never re-ranking or reordering, only removing non-matching entries.

---

## Objective 3

Object search results display Object ID, Object Type, Name, Match Score, Match Location. Relationship search results display Relationship ID, Relationship Type, Source Object ID, Target Object ID, Match Score, Match Location.

---

## Objective 4

Gracefully report: repository not found, empty repository, no matching results, invalid command syntax — all via descriptive errors, none exposing internal implementation details.

---

## Objective 5

Register `SearchCommand` so Help lists it consistently with existing commands.

---

## Objective 6

Update `platform/runtime/CLI_USAGE.md` with search command syntax, available filters, example searches, expected output, and current limitations.

---

## Objective 7

Add unit tests validating object search, relationship search, filtering (type/author/tag), empty repositories, invalid repositories, no-match results, and Runtime integration.

---

# Explicitly Out of Scope

Do not implement:

- Semantic search
- AI-assisted search
- Regular expression search
- Saved searches
- Advanced query languages
- Runtime, SDK, Studios, Exchange, Networking, Authentication, Plugin system, GUI

These systems belong to future tasks.

---

# Deliverables

- `SearchCommand` (`oep search`, `oep search objects`, `oep search relationships`)
- Updated `platform/runtime/CLI_USAGE.md`
- Unit tests

---

# Acceptance Criteria

This task is complete only when:

- The project builds successfully.
- Object search executes successfully.
- Relationship search executes successfully.
- Search filters operate correctly.
- Runtime integration is preserved (no direct `SearchEngine` construction in the CLI).
- Search results remain deterministic (CLI never re-ranks).
- Help lists the `search` command.
- `CLI_USAGE.md` is updated.
- Unit tests covering object search, relationship search, filtering, empty repositories, invalid repositories, no-match results, and Runtime integration pass.

---

# Engineering Expectations

Favor readability.

Favor maintainability.

Favor simplicity.

The CLI never instantiates `SearchEngine` directly and never implements independent ranking — all search logic stays owned by the Search subsystem.

Avoid speculative implementation (no semantic search, no regex, no saved searches).

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

- Build: succeeded, including the new `SearchCommand`
- `oep_repository_tests`, `oep_engineering_object_tests`, `oep_relationship_tests`, `oep_graph_engine_tests`, `oep_audit_store_tests`, `oep_search_engine_tests`, `oep_repository_validator_tests`, `oep_package_manager_tests`, `oep_foundation_runtime_tests`, `oep_cli_commands_tests`, `oep_object_command_tests`, `oep_relationship_command_tests`, `oep_search_command_tests` (CTest): 13/13 suites passed
- Manual smoke test: bare query, `search objects`, `search relationships`, a `--type` filter, a no-match query, and an invalid-repository case all run correctly against a real generated repository
- `platform/runtime/CLI_USAGE.md`: updated with the search command/filter tables, a full example session with real captured output, and new limitations

Task 000015 is complete pending formal acceptance.
