# CURRENT_SPRINT
# CURRENT_SPRINT.md
## Open Engineering Platform (OEP)

Sprint: 012

Status: Active

---

# Sprint Name

CLI Command Framework Expansion

---

# Sprint Objective

Implement the CLI Command Framework Expansion per OEP-SPEC-012-CLI_COMMAND_FRAMEWORK: expand `oep` into the primary developer interface for Foundation via `open`/`validate`/`packages`/`status`, all backed exclusively by `FoundationRuntime`.

The goal is not to build an interactive shell, a scripting layer, or structured output.

The goal is for the CLI to be a thin presentation layer over Foundation — no command duplicates Foundation business logic.

---

# Primary Deliverable

Four new commands + `platform/runtime/CLI_USAGE.md`

`platform/cli` is refactored into a library (`oep_cli_core`) plus a thin executable, so commands can be unit-tested directly.

---

# Sprint Scope

This sprint includes:

- `oep_cli_core` library refactor
- `open`/`validate`/`packages`/`status` commands
- Extended help system (Name/Syntax/Description)
- `platform/runtime/CLI_USAGE.md`
- Consolidating the duplicated Foundation version string into one constant
- Unit tests
- Documentation updates

---

# Out of Scope

The following items are explicitly excluded from this sprint:

- Interactive shell mode
- Scripting language support
- Remote execution
- Studio user interfaces
- Structured/JSON command output
- Detailed per-command help beyond Name/Syntax/Description
- SDK
- Exchange
- Authentication
- Plugin System
- GUI

If implementation of these items appears necessary, document the dependency and continue working within the current sprint boundaries.

---

# Acceptance Criteria

This sprint is complete when:

- The project builds successfully.
- `oep open`, `oep validate`, `oep packages`, and `oep status` all function correctly through Foundation Runtime.
- Help lists every registered command with Name/Syntax/Description.
- No command constructs Repository/Search/Validation/Package services directly.
- `platform/runtime/CLI_USAGE.md` exists and is accurate.
- Unit tests covering command execution, Runtime integration, invalid-repository handling, help generation, and error reporting pass.
- Documentation is updated.

---

# Definition of Success

At the conclusion of this sprint:

A new developer should be able to clone the repository, build the CLI, execute it, understand the architecture, and be prepared to begin implementing additional commands.

---

# Engineering Principles

During this sprint:

Build only what is necessary.

Avoid feature creep.

Preserve architectural boundaries.

Favor maintainability over speed.

Prefer modularity over convenience.

Do not optimize prematurely.

---

# Deliverables

- OEP CLI executable
- Command registry
- Command interface
- Version command
- Help command
- Initial template framework
- Updated documentation

---

# Risks

Primary risks during this sprint:

- Over-engineering
- Introducing unnecessary dependencies
- Expanding sprint scope
- Breaking architectural boundaries

If any of these occur, stop and reevaluate before continuing.

---

# Next Sprint Preview

Sprint 002

Foundation Generator

Objectives:

- Implement `oep init`
- Generate complete OEP repositories
- Create official repository template
- Validate generated repository
- Prepare for OEP Shell development

This section is informational only and shall not influence implementation during the current sprint.

---

# Completion Review

Before closing the sprint, verify:

✓ Project builds

✓ Documentation updated

✓ Architecture preserved

✓ Repository cleaner than before

✓ Acceptance criteria satisfied

Only then may the sprint be considered complete.

---

# Task 000001 — Verified Complete

`oep --help` and `oep version` were built with MSVC 19.51 (Visual Studio Build Tools 18) via CMake/Ninja and executed successfully (exit code 0 for both). Sprint 001 acceptance criteria are satisfied.

---

# Task 000002 — Verified Complete

`oep init my-workshop` was built and executed with MSVC 19.51 via CMake/Ninja. The generated repository matched OEP-SPEC-002-FOUNDATION_REPOSITORY exactly: `repository/`, `workspace/`, `packages/`, `cache/`, `logs/`, `exports/`, `settings/`, plus `README.md`, `.gitignore`, `repository.json`, and `workspace.json` with a valid UUIDv4 repository ID. Re-running `init` against the same (now non-empty) directory failed safely with exit code 1 and made no changes. Sprint 002 acceptance criteria are satisfied.

---

# Task 000003 — Verified Complete

The `platform/repository` metadata module (schema, loader, writer, validation, internal JSON parser/serializer) was built with MSVC 19.51 via CMake/Ninja. `oep_repository_tests` (6 cases covering valid load, invalid JSON, missing fields, bad UUID, save/round-trip with timestamp refresh, and rejection of invalid metadata) passed via CTest. `oep init` now populates `repository.json` through the new metadata writer, verified end-to-end to match OEP-SPEC-003's schema with no leftover temporary file. Sprint 003 acceptance criteria are satisfied.

---

# Task 000004 — Verified Complete

Added `EngineeringObject`, `ObjectType`, validation, JSON serialization, and a Create/Read/Update/Delete `ObjectStore` to `platform/repository`. UUID generation/validation and UTC timestamp formatting — previously duplicated between the CLI generator and the metadata module — were consolidated into shared `oep/repository/uuid.hpp` and `oep/repository/timestamp.hpp`; the CLI's local `uuid.cpp`/`uuid.hpp` were removed. Built with MSVC 19.51 via CMake/Ninja; `oep_engineering_object_tests` (8 cases: create assigns id/timestamps, load round-trip, missing-object load failure, update preserving immutable fields and refreshing lastModifiedUtc, remove, and validation rejections) passed via CTest alongside the existing metadata tests (2/2 suites). `oep init` re-verified end-to-end after the refactor. Sprint 004 acceptance criteria are satisfied.

---

# Task 000005 — Verified Complete

Added `Relationship`, `RelationshipType`, validation, JSON serialization, and a Create/Read/Update/Delete/Enumerate `RelationshipStore` to `platform/repository`, validated against an `ObjectStore` so relationships can only be created between Engineering Objects that actually exist. Built with MSVC 19.51 via CMake/Ninja; `oep_relationship_tests` (9 cases: create succeeds for existing objects, create fails for a missing object, create fails when source equals target, load round-trip, update preserving immutable fields, remove, enumerate-by-object in both directions, and two validation rejections) passed via CTest alongside the existing metadata and object test suites (3/3 suites). `oep init` re-verified unaffected. Sprint 005 acceptance criteria are satisfied.

---

# Task 000006 — Verified Complete

Filled in `platform/search` (previously a scaffolded, empty module) with `SearchEngine`: `build_index`, `clear_index`, `search_objects`, `search_relationships`, and result types with match location and score. Added `ObjectStore::list_all` and `RelationshipStore::list_all` to `platform/repository` to support index construction, and refactored `list_by_object` to reuse `list_all` rather than duplicating enumeration. Built with MSVC 19.51 via CMake/Ninja; `oep_search_engine_tests` (covering exact/partial/case-insensitive matches across name, description, author, tags, and object type for objects and relationship type/description/author for relationships, missing-index and empty-repository handling, empty-query rejection, determinism across repeated searches, and index rebuild after object removal) passed via CTest alongside the existing three suites (4/4 suites). `oep init` re-verified unaffected. Sprint 006 acceptance criteria are satisfied.

---

# Task 000007 — Verified Complete

Added `GraphEngine` to `platform/repository` (no dedicated `platform/graph` module exists in the frozen architecture, and the spec frames this as a Repository capability): `build_graph`, `clear_graph`, `get_neighbors`, `traverse_breadth_first`, `traverse_depth_first`, `path_exists`, built from `ObjectStore`/`RelationshipStore` via their existing `list_all`. Relationships are treated as connecting their two objects bidirectionally for traversal purposes while preserving the original relationship type/ID on each edge; relationships referencing a missing object are excluded from the graph. Built with MSVC 19.51 via CMake/Ninja; `oep_graph_engine_tests` (covering neighbor discovery with relationship-type preservation, an isolated node, BFS/DFS over a 3-node cycle without infinite looping, determinism across repeated traversals, path existence for connected/disconnected/self pairs, nonexistent-object handling, graph rebuild after object removal, `clear_graph` isolation from repository state, and an empty repository) passed via CTest alongside the existing four suites (5/5 suites). `oep init` re-verified unaffected. Sprint 007 acceptance criteria are satisfied.

---

# Task 000008 — Verified Complete

Added `AuditEvent`/`AuditEventType` and `AuditStore` to `platform/repository`, then changed `ObjectStore`'s and `RelationshipStore`'s constructors to require an `AuditStore`, so every successful create/update/remove automatically records `ObjectCreated`/`ObjectUpdated`/`ObjectDeleted` or `RelationshipCreated`/`RelationshipUpdated`/`RelationshipDeleted` — applications cannot bypass the audit trail by construction. Audit recording is best-effort and never causes an otherwise-successful operation to fail. `AuditEventType::RepositoryCreated` is supported but intentionally not wired into `oep init`, since OEP-SPEC-002's frozen repository layout doesn't define where an audit log lives on disk. Built with MSVC 19.51 via CMake/Ninja; `oep_audit_store_tests` (record/list/list-for-object, validation rejections, `clear`, and two integration tests confirming `ObjectStore` and `RelationshipStore` each auto-record exactly one event per create/update/remove) passed via CTest alongside the existing five suites (6/6 suites). `oep init` re-verified unaffected. Sprint 008 acceptance criteria are satisfied.

---

# Task 000009 — Verified Complete

Filled in `platform/validation` (previously a scaffolded, empty module) with `RepositoryValidator`: `validate_metadata`, `validate_objects`, `validate_relationships`, and `validate_repository`, producing a deterministic `ValidationReport` (Severity/Category/Message/ObjectId findings, plus error/warning/information counts and a Healthy/Unhealthy status). Extended `ObjectStore`/`RelationshipStore`'s `list_all` to report which stored files failed to parse and why (`invalid_entries`), since they previously discarded that information silently — without this, "no invalid object/relationship types exist" could not actually be checked. Built with MSVC 19.51 via CMake/Ninja; `oep_repository_validator_tests` (12 cases covering a healthy repository, each of the ten integrity rules individually via targeted corruption — including cases requiring raw file writes to simulate corruption unreachable through the normal store APIs — validation continuing past multiple simultaneous errors, and report determinism) passed via CTest alongside the existing six suites (7/7 suites). `oep init` re-verified unaffected. Sprint 009 acceptance criteria are satisfied.

---

# Task 000010 — Verified Complete

Added a new `platform/packages` module (no pre-scaffolded placeholder existed for it, unlike Search/Validation) with `PackageManifest`/`PackageType`, validation, and `PackageManager` (`discover_packages`/`load_package`/`list_packages`), per OEP-SPEC-010-PACKAGE_SYSTEM. Promoted `platform/repository`'s internal JSON parser (`src/json_value.hpp`) to a public header (`include/oep/repository/json_value.hpp`) so packages could parse `package.json` without a second JSON implementation. Foundation-version compatibility is checked at major.minor granularity against a version supplied to `PackageManager` by its caller, since no canonical "current Foundation version" constant exists yet. Duplicate packageId detection is implemented in `list_packages` (the only operation with visibility across sibling packages), demoting every package after the first to claim an ID to Invalid. Built with MSVC 19.51 via CMake/Ninja; `oep_package_manager_tests` (9 cases: deterministic discovery, valid load, missing/malformed/invalid-field manifests, unsupported Foundation version, `.disabled` marker handling, duplicate packageId detection, and an invalid package not blocking a valid sibling) passed via CTest alongside the existing seven suites (8/8 suites). `oep init` re-verified unaffected. Sprint 010 acceptance criteria are satisfied.

---

# Task 000011 — Verified Complete

Filled in `platform/runtime` (previously a scaffolded, empty module) with `FoundationRuntime`, per OEP-SPEC-011-FOUNDATION_RUNTIME: `RuntimeState` (Uninitialized/Initialized/RepositoryOpen/RepositoryClosed/Shutdown), deterministic `initialize`/`open_repository`/`close_repository`/`shutdown` transitions, Repository Context accessors (current repository/metadata/package set), and a Service Registry exposing `ObjectStore`/`RelationshipStore`/`AuditStore`/`SearchEngine`/`RepositoryValidator`/`PackageManager` — all constructed and coordinated, none reimplemented. `open_repository` establishes, for the first time, where Engineering Objects/Relationships/Audit Events live within an opened repository (`repository/objects`, `repository/relationships`, `repository/audit`); this is deliberately not wired into `oep init` in this task. Built with MSVC 19.51 via CMake/Ninja; `oep_foundation_runtime_tests` (8 cases: initialize/reinitialize, open-before-initialize rejection, full open/close lifecycle, missing-metadata rejection leaving state unchanged, rejecting a second concurrent open while preserving the first as current, reopening a different repository after close, all six services and all three context accessors being available only while a repository is open, and shutdown idempotency/auto-close/post-shutdown rejection) passed via CTest alongside the existing eight suites (9/9 suites). `oep init` re-verified unaffected. Sprint 011 acceptance criteria are satisfied.

---

# Task 000012 — Verified Complete

Refactored `platform/cli` into a static library (`oep_cli_core`) plus a thin executable, so command logic is directly unit-testable without spawning the built binary as a subprocess. Added `open`/`validate`/`packages`/`status`, each obtaining services exclusively through `FoundationRuntime` (initialize → open_repository → do the work → shutdown), per OEP-SPEC-012-CLI_COMMAND_FRAMEWORK. Extended `Command` with a `usage()` method and `HelpCommand` to print Name/Syntax/Description for every registered command. Consolidated the Foundation version string — previously duplicated between `VersionCommand` and the Foundation Generator — into one `oep::cli::kFoundationVersion` constant, now also passed to every `FoundationRuntime` a command constructs. Wrote `platform/runtime/CLI_USAGE.md` per the spec's Definition-of-Done requirement. Built with MSVC 19.51 via CMake/Ninja; `oep_cli_commands_tests` (9 cases: open succeeding/failing/reporting a descriptive non-crash-looking error, validate reporting Healthy and failing gracefully for a missing repository, packages reporting none discovered, status reporting a fully-populated open-repository summary and a graceful no-repository summary, and help listing every command with syntax and description) passed via CTest alongside the existing nine suites (10/10 suites). Manually smoke-tested every new command plus an invalid-repository case. `oep init` re-verified unaffected. Sprint 012 acceptance criteria are satisfied.