# TASK
# TASK.md
## Open Engineering Platform (OEP)

Task ID: 000010

Status: Complete

---

# Current Task

Implement the Package System Foundation per OEP-SPEC-010-PACKAGE_SYSTEM.

This delivers a `PackageManifest` model and a `PackageManager` capable of discovering, loading, and validating packages under a repository's `packages/` directory.

---

# Context

TASK-000009 (Repository Integrity Validation) is complete and accepted.

OEP-SPEC-010-PACKAGE_SYSTEM.md defines package identity/metadata, the five initial package types, the manifest (`package.json`) requirement, the `DiscoverPackages`/`LoadPackage`/`ListPackages` interface, validation rules, and the Loaded/Invalid/Disabled package states. Package installation, an online registry, updates, and dependencies are explicitly deferred to future specifications.

No `platform/packages` module is pre-scaffolded in the frozen architecture (unlike Search and Validation, which had empty placeholders). This task adds it as a new module, since the spec describes a distinct Foundation subsystem ("Foundation shall expose a Package Manager") rather than a capability of an existing one.

While implementing, promoted `platform/repository`'s internal JSON parser (`src/json_value.hpp`) to a public header (`include/oep/repository/json_value.hpp`) so `platform/packages` can parse `package.json` without a second, duplicate JSON implementation.

---

# Objectives

Complete the following tasks only.

## Objective 1

Add `PackageManifest`/`PackageType` to `platform/packages`: `packageId`, `packageName`, `packageVersion`, `packageType`, `author`, `organization`, `description`, `tags`, `foundationVersion`, `createdUtc`, plus the five initial package types (EngineeringContent, Studio, SDK, Extension, Theme).

---

## Objective 2

Implement validation: required fields, UUIDv4 packageId, semantic-version packageVersion/foundationVersion, valid packageType.

---

## Objective 3

Implement `PackageManager`: `discover_packages` (find candidate package directories under `packages/`, deterministically), `load_package` (load and validate one package by directory name), `list_packages` (discover + load every candidate). Foundation version compatibility is checked against a version supplied to `PackageManager`, not inferred or hardcoded, since nothing in the codebase currently owns "the current Foundation version" as a canonical constant.

---

## Objective 4

Package state (Loaded/Invalid/Disabled) is reported per discovered package. A package directory containing a `.disabled` marker file is Disabled; otherwise a package with a missing/malformed/incompatible manifest is Invalid; otherwise Loaded. An invalid package never prevents other valid packages from being discovered or loaded.

---

## Objective 5

Add unit tests validating discovery, validation, loading, duplicate package IDs, invalid manifests, and unsupported Foundation versions.

---

# Explicitly Out of Scope

Do not implement:

- Package installation
- Online package registry
- Package updates
- Package dependencies
- Runtime, SDK, Studios, Exchange, Networking, Authentication, Plugin system, GUI

These systems belong to future tasks.

---

# Deliverables

- `platform/packages` module (`PackageManifest`, `PackageManager`, package state)
- Unit tests

---

# Acceptance Criteria

This task is complete only when:

- The project builds successfully.
- Package discovery succeeds.
- Valid packages load.
- Invalid packages are rejected without blocking valid ones.
- Package manifests are validated.
- Unit tests covering discovery, validation, loading, duplicate package IDs, invalid manifests, and unsupported Foundation versions pass.

---

# Engineering Expectations

Favor readability.

Favor maintainability.

Favor simplicity.

No external JSON library dependency — reuse `platform/repository`'s JSON parser rather than writing a second one.

Avoid speculative implementation (no installation, registry, updates, or dependency resolution).

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

- Build: succeeded, including the new `platform/packages` module and the `json_value.hpp` promotion to a public `platform/repository` header
- `oep_repository_tests`, `oep_engineering_object_tests`, `oep_relationship_tests`, `oep_graph_engine_tests`, `oep_audit_store_tests`, `oep_search_engine_tests`, `oep_repository_validator_tests`, `oep_package_manager_tests` (CTest): 8/8 suites passed
  - Package suite (9 cases) covers: deterministic discovery (ignoring directories without a manifest), a valid package loading successfully, a missing manifest, invalid JSON, invalid manifest fields (bad UUID, empty name), an unsupported Foundation version, a `.disabled` marker overriding an otherwise-valid manifest, duplicate packageId detection across two packages, and an invalid package not blocking a valid sibling
- `oep init my-workshop`: re-verified unaffected, exit code 0

Task 000010 is complete pending formal acceptance.
