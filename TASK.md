# TASK
# TASK.md
## Open Engineering Platform (OEP)

Task ID: 000016

Status: Complete

---

# Current Task

Implement Repository Graph CLI Commands per OEP-SPEC-016-GRAPH_COMMANDS.

This delivers `oep graph neighbors|traverse|path`, exposing the existing `GraphEngine` (implemented in Sprint 007) through Foundation Runtime.

---

# Context

TASK-000015 (Repository Search CLI Commands) is complete and accepted.

OEP-SPEC-016-GRAPH_COMMANDS.md requires every graph command to go through Foundation Runtime and never instantiate the Graph Engine directly — but `FoundationRuntime` (Sprint 011) never registered `GraphEngine` as a service in the first place; only `ObjectStore`/`RelationshipStore`/`AuditStore`/`SearchEngine`/`RepositoryValidator`/`PackageManager` were wired in. This task extends `FoundationRuntime` with a `graph_engine()` accessor (built via `GraphEngine::build_graph` during `open_repository`, reset on close/shutdown, following the exact pattern the other five services already use) before building the CLI command on top of it.

---

# Objectives

Complete the following tasks only.

## Objective 1

Extend `FoundationRuntime` to build and expose a `GraphEngine`, consistent with how it already builds and exposes `SearchEngine`.

---

## Objective 2

Add `GraphCommand` (registered as `graph`) to `platform/cli`, dispatching to `neighbors <object-id>`, `traverse <object-id>`, `path <source-object-id> <target-object-id>`.

---

## Objective 3

`neighbors` displays Neighbor Object ID, Neighbor Object Type, Neighbor Object Name, Relationship Type. `traverse` supports `--algorithm bfs` (default) / `--algorithm dfs` and displays Traversal Order, Object ID, Object Type, Object Name. `path` reports only Path Found / No Path Found.

---

## Objective 4

Register `GraphCommand` so Help lists it consistently with existing commands.

---

## Objective 5

Update `platform/runtime/CLI_USAGE.md` with graph command syntax, BFS/DFS usage, neighbor discovery examples, path detection examples, expected output, and current limitations.

---

## Objective 6

Add unit tests validating neighbor discovery, BFS traversal, DFS traversal, disconnected graphs, invalid object IDs, path detection, empty repositories, and Runtime integration (including that `FoundationRuntime::graph_engine()` follows the same open/close availability pattern as the other five services).

---

# Explicitly Out of Scope

Do not implement:

- Graph visualization
- Shortest-path algorithms
- Weighted traversal
- Interactive graph exploration
- Graph editing
- Runtime, SDK, Studios, Exchange, Networking, Authentication, Plugin system, GUI

These systems belong to future tasks.

---

# Deliverables

- `FoundationRuntime::graph_engine()` accessor
- `GraphCommand` (`neighbors`/`traverse`/`path`)
- Updated `platform/runtime/CLI_USAGE.md`
- Unit tests

---

# Acceptance Criteria

This task is complete only when:

- The project builds successfully.
- Neighbor discovery, BFS traversal, DFS traversal, and path detection all execute successfully.
- Runtime integration is preserved — no command constructs `GraphEngine` (or any Repository service) directly.
- Help lists the `graph` command.
- `CLI_USAGE.md` is updated.
- Unit tests covering neighbor discovery, BFS/DFS traversal, disconnected graphs, invalid object IDs, path detection, empty repositories, and Runtime integration pass.

---

# Engineering Expectations

Favor readability.

Favor maintainability.

Favor simplicity.

The CLI never implements traversal logic itself — all graph algorithms stay owned by `GraphEngine` in `platform/repository`.

Avoid speculative implementation (no visualization, no shortest-path, no weighted/interactive traversal).

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

- Build: succeeded, including `FoundationRuntime::graph_engine()` and the new `GraphCommand`
- `oep_repository_tests`, `oep_engineering_object_tests`, `oep_relationship_tests`, `oep_graph_engine_tests`, `oep_audit_store_tests`, `oep_search_engine_tests`, `oep_repository_validator_tests`, `oep_package_manager_tests`, `oep_foundation_runtime_tests`, `oep_cli_commands_tests`, `oep_object_command_tests`, `oep_relationship_command_tests`, `oep_search_command_tests`, `oep_graph_command_tests` (CTest): 14/14 suites passed
- Manual smoke test: `graph neighbors`/`graph traverse` (both BFS and DFS)/`graph path` (found and not-found) all run correctly against a real four-object, two-relationship generated repository, plus an invalid-object-ID error case
- `platform/runtime/CLI_USAGE.md` and `platform/runtime/README.md`: updated with the graph command tables, BFS/DFS explanation, a full example session with real captured output, and new limitations

Task 000016 is complete pending formal acceptance.
