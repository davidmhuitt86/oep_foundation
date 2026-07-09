# CURRENT_SPRINT
# CURRENT_SPRINT.md
## Open Engineering Platform (OEP)

Sprint: 019-020

Status: Complete

---

# Sprint Name

Repository Templates & Batch Operations (Work Package 010)

---

# Sprint Objective

Implement Repository Graph CLI Commands per OEP-SPEC-016-GRAPH_COMMANDS: `oep graph neighbors|traverse|path`, backed exclusively by Foundation Runtime and the existing `GraphEngine`.

The goal is not to build graph visualization, shortest-path algorithms, or interactive exploration.

The goal is to make the connections between Engineering Objects navigable from the CLI, while all graph algorithms stay owned by the Repository subsystem.

---

# Primary Deliverable

`GraphCommand` + `FoundationRuntime::graph_engine()`

`FoundationRuntime` gains a sixth registered service (`GraphEngine`, built the same way `SearchEngine` already is); `GraphCommand` is a thin layer over it.

---

# Sprint Scope

This sprint includes:

- `FoundationRuntime::graph_engine()`
- `graph neighbors <object-id>` / `graph traverse <object-id> [--algorithm bfs|dfs]` / `graph path <source> <target>`
- Updated `platform/runtime/CLI_USAGE.md`
- Unit tests

---

# Out of Scope

The following items are explicitly excluded from this sprint:

- Graph visualization
- Shortest-path algorithms
- Weighted traversal
- Interactive graph exploration
- Graph editing
- Runtime
- SDK
- Studios
- Exchange
- Networking
- Authentication
- Plugin System
- GUI

If implementation of these items appears necessary, document the dependency and continue working within the current sprint boundaries.

---

# Acceptance Criteria

This sprint is complete when:

- The project builds successfully.
- Neighbor discovery, BFS traversal, DFS traversal, and path detection all execute successfully.
- Runtime integration is preserved; the CLI never constructs `GraphEngine` directly.
- Help lists the `graph` command.
- `CLI_USAGE.md` is updated.
- Unit tests covering neighbor discovery, BFS/DFS traversal, disconnected graphs, invalid object IDs, path detection, empty repositories, and Runtime integration pass.
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

---

# Task 000013 — Verified Complete

Added `oep object create|list|show|delete` to `platform/cli`, per OEP-SPEC-013-ENGINEERING_OBJECT_COMMANDS, each subcommand a thin layer over `FoundationRuntime`/`ObjectStore` with no duplicated business logic. `create` accepts `--type`/`--name` (required) plus `--description`/`--author`/`--tags`; `list` prints Object ID/Type/Name/Version sorted by ID for determinism; `show` prints every field including both timestamps; `delete` removes through the existing `ObjectStore::remove`, which already auto-records an `ObjectDeleted` Audit Event (no new audit wiring needed — confirmed by a dedicated test reading the event back out of the `AuditStore`). Repository selection uses an optional `--repository <path>` flag (defaulting to cwd) rather than a positional argument, since `show`/`delete` already use their one positional slot for the object ID. Updated `platform/runtime/CLI_USAGE.md` with the new command table, example workflow, and limitations, per the spec's Definition-of-Done requirement. Built with MSVC 19.51 via CMake/Ninja; `oep_object_command_tests` (7 cases: missing required create fields, an unrecognized object type, a full create→list→show→delete→show-after-delete lifecycle, the automatic audit event on delete, show failing for a nonexistent ID, list reporting none for an empty repository, and an unknown subcommand being rejected) passed via CTest alongside the existing ten suites (11/11 suites). Manually smoke-tested the full lifecycle against a real generated repository. `oep init` re-verified unaffected. Sprint 013 acceptance criteria are satisfied.

---

# Task 000014 — Verified Complete

Added `oep relationship create|list|show|delete` to `platform/cli`, per OEP-SPEC-014-RELATIONSHIP_COMMANDS, mirroring `ObjectCommand` exactly and backed by `RelationshipStore`. Factored the `--repository <path>` extraction helper (previously duplicated logic if copy-pasted) into a shared `repository_path_option.hpp/.cpp` used by both `object` and `relationship`. `create` requires `--source`/`--target`/`--type` and relies entirely on `RelationshipStore::create`'s existing validation (object existence, source ≠ target, valid type) — no CLI-side re-validation. `delete` removes through the existing `RelationshipStore::remove`, which already auto-records a `RelationshipDeleted` Audit Event, confirmed by a dedicated test. Updated `platform/runtime/CLI_USAGE.md` with the relationship command table, supported types, a full object+relationship workflow, and a validation-rejection example with real captured error text. Built with MSVC 19.51 via CMake/Ninja; `oep_relationship_command_tests` (8 cases: missing required create fields, unrecognized relationship type, nonexistent object reference, matching source/target rejection, a full create→list→show→delete→show-after-delete lifecycle, the automatic audit event on delete, list reporting none for an empty repository, and an unknown subcommand being rejected) passed via CTest alongside the existing eleven suites (12/12 suites). Manually smoke-tested the full lifecycle, including building a two-object relationship, against a real generated repository. `oep init` re-verified unaffected. Sprint 014 acceptance criteria are satisfied.

---

# Task 000015 — Verified Complete

Added `oep search [objects|relationships] <query>` to `platform/cli`, per OEP-SPEC-015_SEARCH_COMMANDS, exposing the existing `SearchEngine` through `FoundationRuntime` with no independent CLI ranking — filtering happens after `SearchEngine` returns results and only ever removes entries, never reorders them. Reused the shared `--repository` helper from Sprint 014. `--type`/`--author` filter both object and relationship results by loading each hit's full record; `--tag` only applies to objects, since `Relationship` has no tags field (documented rather than silently ignored or erroring). Updated `platform/runtime/CLI_USAGE.md` with the search command/filter tables and a full example session with real captured output. Built with MSVC 19.51 via CMake/Ninja; `oep_search_command_tests` (10 cases: object search, relationship search, a bare query searching both, the three filters each individually excluding a non-matching entry, no-match handling, an empty repository, an invalid repository, and a missing query) passed via CTest alongside the existing twelve suites (13/13 suites). Manually smoke-tested bare/objects/relationships forms, a type filter, and an invalid-repository case against a real generated repository. `oep init` re-verified unaffected. Sprint 015 acceptance criteria are satisfied.

---

# Task 000016 — Verified Complete

Extended `FoundationRuntime` with a sixth registered service, `graph_engine()`, built via `GraphEngine::build_graph` during `open_repository` exactly the way `SearchEngine` already is — this was a real gap: the spec required graph commands to never construct `GraphEngine` directly, but Runtime never exposed one at all until now. Added `oep graph neighbors|traverse|path` to `platform/cli`, per OEP-SPEC-016-GRAPH_COMMANDS, each subcommand a thin layer over the new accessor: `neighbors` lists directly connected objects with their relationship type; `traverse` supports `--algorithm bfs` (default) / `dfs`, delegating entirely to `GraphEngine`'s existing traversal; `path` reports only Path Found / No Path Found. Updated `platform/runtime/CLI_USAGE.md` and `platform/runtime/README.md` (service list) with the graph command/filter tables and a full four-object example session with real captured output, including a disconnected node. Built with MSVC 19.51 via CMake/Ninja; `oep_graph_command_tests` (10 cases: neighbor listing, an isolated object reporting none, BFS and DFS traversal over a connected component that correctly excludes a disconnected node, path found/not-found, an invalid object ID, an empty repository, an invalid repository, and an unknown subcommand) passed via CTest alongside the existing thirteen suites (14/14 suites). Manually smoke-tested neighbors/traverse (both algorithms)/path against a real four-object, two-relationship graph, including the disconnected-node case. `oep init` re-verified unaffected. Sprint 016 acceptance criteria are satisfied.

---

# Work Package 009 (Task 000017 + Task 000018) — Verified Complete

Filled in `platform/exchange` (previously a scaffolded, empty module matching PROJECT_MEMORY's "Exchange distributes Repository content" description) with `ExportManifest`, `export_repository`, and `import_repository`, per OEP-SPEC-017-REPOSITORY_EXPORT and OEP-SPEC-018-REPOSITORY_IMPORT. The export archive is a single deterministic JSON document (metadata, objects, relationships, audit log, optional package manifests, and the export manifest itself) rather than a zip/tar, avoiding a new compression-library dependency while still satisfying "a single archive."

Along the way, added `restore()` to `ObjectStore`, `RelationshipStore`, and `AuditStore` (additive-only — existing `create`/`update`/`remove`/`list_all` signatures and behavior are unchanged). This was necessary, not optional: `create()` always regenerates `lastModifiedUtc` and would corrupt round-trip fidelity if reused for import, so faithful reconstruction needed a write-exactly-as-given path that records no audit event (restoring history is not creating new history).

Extended `FoundationRuntime` with `export_repository` (requires an open repository, delegates entirely to `oep::exchange::export_repository`) and `import_repository` (does not require or use an open repository — it targets a destination to create — but is still exposed on the Runtime so import continues to execute "entirely through Foundation Runtime" as both specs require, rather than the CLI reconstructing repository contents directly).

Added `oep export <output-file> [--include-packages] [--repository <path>]` and `oep import <archive-file> [--destination <path>] [--overwrite]` to `platform/cli`. Generalized the `--repository`-only flag helper (from Sprint 014) into `extract_path_option`/`extract_flag`, non-breaking, so `--include-packages`, `--destination`, and `--overwrite` all share the same primitive `extract_repository_path` is now built on.

Updated `platform/runtime/CLI_USAGE.md` (export/import command tables, archive format and validation-order explanation, a full export→import round-trip example with real captured output including exact ID/timestamp preservation, overwrite-rejection and invalid-archive examples, and new limitations), `platform/runtime/README.md` (service list), `platform/cli/README.md`, and `platform/exchange/README.md` (from Placeholder to Foundation).

Built with MSVC 19.51 via CMake/Ninja after a full clean rebuild (`rm -rf build` then reconfigure); `oep_repository_exporter_tests` (4 cases), `oep_repository_importer_tests` (7 cases: successful import preserving identity, overwrite rejection/acceptance, invalid JSON, missing archive, corrupted manifest, unsupported export version, package restoration), `oep_export_command_tests` (4 cases), and `oep_import_command_tests` (5 cases, including a full round-trip through the real CLI commands) all passed via CTest alongside the existing fourteen suites (18/18 suites). One test bug was caught and fixed during verification: a test helper's `sample_metadata()` never set `last_modified_utc`, correctly tripping `validate_metadata`'s required-field check during import — not an exchange-module bug. Manually smoke-tested the complete export→import round trip (exact object ID/author/tags/timestamps preserved), overwrite protection, and invalid-archive rejection against real generated repositories. `oep init` re-verified unaffected. Sprint 017/018 acceptance criteria are satisfied.

---

# Work Package 010 (Task 000019 + Task 000020) — Verified Complete

**Task 000019 — Repository Templates.** Added `TemplateManifest` (Template ID, Name, Version, Description, Author, Tags, Foundation Version) and `TemplateStore` (`create_template`/`list_templates`/`instantiate_template`) to `platform/exchange`, per OEP-SPEC-019-REPOSITORY_TEMPLATES. A template is a single JSON file reusing the same archive codec Export/Import already produce (extended with `TemplateManifest` marshaling) rather than a new format — a template is "a named, reusable archive with its own manifest." `create_template` validates the manifest before writing; `list_templates` silently excludes any file that fails validation rather than crashing the listing; `instantiate_template` fully loads and validates the template *before* touching the destination, rejects a non-empty existing destination, generates a **new** repository ID for the instantiated copy while restoring every object/relationship with its **exact original ID** via the `restore()` path added in Work Package 009. Extended `FoundationRuntime` with `create_template`/`list_templates`/`instantiate_template`, all delegating entirely to `TemplateStore`. Added `oep template create|list|instantiate` to `platform/cli`, reusing the shared `--repository`/path-option helpers and a new shared `split_csv` (extracted from `object_command`'s previously-duplicated `split_tags`, per the work package's pre-authorized shared refactoring).

**Task 000020 — Batch Operations.** Added `BatchProcessor` to `platform/repository` (not `platform/exchange` — bulk CRUD against an *already-open* repository is Repository-domain business logic, like `RepositoryValidator`/`GraphEngine`, distinct from Export/Import/Templates' whole-repository packaging concern), per OEP-SPEC-020-BATCH_OPERATIONS. Defines a deterministic JSON batch format for both create (`objects`/`relationships`, with a batch-local `ref` alias so a relationship can reference an object created earlier in the same batch) and delete (`objectIds`/`relationshipIds`). `validate_batch_create_request` runs before any execution and reports duplicate refs, missing names, unresolvable source/target references, and self-referencing relationships, deterministically; `execute_batch_create`/`execute_batch_delete` both validate fully before writing anything — a batch with any problem creates or deletes **nothing**. Extended `FoundationRuntime` with `execute_batch_create`/`execute_batch_delete`/`validate_batch_create`, all requiring an open repository and taking raw JSON text so the CLI never touches parsed repository types directly. Added `oep batch create|delete|validate <input-file>` to `platform/cli`.

Updated `platform/runtime/CLI_USAGE.md` (template and batch command tables, JSON format documentation, full example sessions with real captured output, new limitations for both), `platform/runtime/README.md`, `platform/cli/README.md`, `platform/repository/README.md`, and `platform/exchange/README.md`.

Built with MSVC 19.51 via CMake/Ninja after a full clean rebuild (`rm -rf build` then reconfigure): 95/95 build steps succeeded. `oep_repository_template_tests` (5 cases: full create→list→instantiate workflow verifying new repository ID with preserved object IDs, instantiate rejecting a nonexistent template, instantiate rejecting an invalid manifest without partially creating the destination, list excluding a corrupt template file, list handling an empty/nonexistent directory), `oep_template_command_tests` (5 cases including a full CLI-level round trip), `oep_batch_processor_tests` (9 cases: malformed JSON rejection for both create and delete, unrecognized type rejection, successful create with in-batch ref resolution, duplicate-ref validation, unknown-reference validation, a failed validation creating nothing, successful delete, a failed delete deleting nothing), and `oep_batch_command_tests` (6 cases covering the full CLI surface) all passed via CTest alongside the existing eighteen suites (22/22 suites, 100% pass). Two compile-time issues were caught and fixed: a missing `#include "oep/repository/timestamp.hpp"` in `repository_template.cpp`, and a missing `#include <map>` plus a wrong include path for `json_value.hpp` in `batch_processor.cpp` (fixed proactively before the first build, based on the equivalent mistake's fix pattern from earlier in the session). Manually smoke-tested template create/list/instantiate (confirming the new repository ID while the object kept its original ID) and batch create/validate/delete (including in-batch ref resolution, duplicate-ref detection, and unknown-reference detection) against real generated repositories. `oep init` re-verified unaffected. Sprint 019/020 acceptance criteria are satisfied.