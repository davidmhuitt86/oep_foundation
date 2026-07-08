# TASK
# TASK.md
## Open Engineering Platform (OEP)

Task ID: 000011

Status: Complete

---

# Current Task

Implement the Foundation Runtime per OEP-SPEC-011-FOUNDATION_RUNTIME.

This delivers a `FoundationRuntime` class that coordinates the existing Repository, Search, Validation, and Package Manager subsystems behind a single lifecycle interface, without reimplementing any of their logic.

---

# Context

TASK-000010 (Package System Foundation) is complete and accepted.

OEP-SPEC-011-FOUNDATION_RUNTIME.md defines the five runtime states, the Initialize/OpenRepository/CloseRepository/Shutdown lifecycle, the service registry (Repository, Search, Validation, Package Manager), and the guards the Runtime must enforce (no concurrent repositories, no service access before initialization or after shutdown).

`platform/runtime` already exists as a scaffolded, empty module per the frozen architecture — this task specifies and fills it in for the first time, the same pattern used for `platform/search` and `platform/validation`.

Scope note: opening a repository requires deciding where, within a generated repository's `repository/` directory, Engineering Objects, Relationships, and Audit Events actually live on disk — a convention no prior sprint established (`ObjectStore`/`RelationshipStore`/`AuditStore` have only ever been exercised against arbitrary test paths). This task establishes that convention (`repository/objects/`, `repository/relationships/`, `repository/audit/`) as part of `FoundationRuntime::open_repository`. It does not wire this into `oep init`/the Foundation Generator — that remains a separate, future integration decision.

---

# Objectives

Complete the following tasks only.

## Objective 1

Add `RuntimeState` (Uninitialized, Initialized, RepositoryOpen, RepositoryClosed, Shutdown) and `FoundationRuntime` to `platform/runtime`.

---

## Objective 2

Implement the lifecycle: `initialize()`, `open_repository(path)`, `close_repository()`, `shutdown()`, with deterministic, descriptive-error-producing state transitions per the state diagram implied by the spec.

---

## Objective 3

`open_repository` loads `repository.json` via the existing metadata loader, then constructs and registers `ObjectStore`, `RelationshipStore`, `AuditStore`, `SearchEngine` (indexed), `RepositoryValidator`, and `PackageManager` for that repository — reusing each subsystem exactly as it already exists, adding no new logic to any of them.

---

## Objective 4

Expose Current Repository, Current Metadata, and Current Package Set, plus accessors for each registered service, all of which are only available while a repository is open (nullptr/failure otherwise — including before initialization and after shutdown or close).

---

## Objective 5

Add unit tests validating initialization, shutdown (including idempotency and auto-closing an open repository), repository open/close lifecycle, invalid state transitions (opening before initializing, opening twice, accessing services before open or after close/shutdown), and that all four services are actually registered and usable once a repository is open.

---

# Explicitly Out of Scope

Do not implement:

- User interface behavior
- Networking
- Cloud synchronization
- AI services
- Multiple concurrent repositories
- Wiring the `repository/objects|relationships|audit` convention into `oep init`
- SDK, Studios, Exchange, Authentication, Plugin system, GUI

These systems belong to future tasks.

---

# Deliverables

- `platform/runtime` module (`FoundationRuntime`, `RuntimeState`)
- Unit tests

---

# Acceptance Criteria

This task is complete only when:

- The project builds successfully.
- The Runtime class exists and its lifecycle functions correctly.
- Repository open/close succeeds.
- All four services are registered and reachable once a repository is open.
- Invalid state transitions are rejected with descriptive errors.
- Unit tests covering initialization, shutdown, repository lifecycle, invalid state transitions, and service registration pass.

---

# Engineering Expectations

Favor readability.

Favor maintainability.

Favor simplicity.

The Runtime coordinates existing subsystems; it must not reimplement Repository, Search, Validation, or Package Manager logic.

Avoid speculative implementation (no multi-repository support, no networking/UI hooks).

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

- Build: succeeded, including the new `platform/runtime` module linking against `oep_repository`, `oep_search`, `oep_validation`, and `oep_packages`
- `oep_repository_tests`, `oep_engineering_object_tests`, `oep_relationship_tests`, `oep_graph_engine_tests`, `oep_audit_store_tests`, `oep_search_engine_tests`, `oep_repository_validator_tests`, `oep_package_manager_tests`, `oep_foundation_runtime_tests` (CTest): 9/9 suites passed
  - Runtime suite (8 cases) covers: initialize/reinitialize, opening before initialization, the full open/close lifecycle, a missing-metadata repository failing to open without corrupting Runtime state, rejecting a second concurrent repository open, reopening a different repository after closing the first, every service and context accessor being available only while a repository is open, and shutdown being idempotent, auto-closing an open repository, and rejecting further opens afterward
- `oep init my-workshop`: re-verified unaffected, exit code 0

Task 000011 is complete pending formal acceptance.
