# TASK
# TASK.md
## Open Engineering Platform (OEP)

Task ID: 000024

Status: Complete

---

# Current Task

Implement the Repository Statistics API per Work Package 012 / TASK-000024 (second of two tasks; follows TASK-000023/Engineering Object Enumeration API immediately, per instruction to complete both before stopping).

This delivers `oep_runtime_get_repository_statistics`, exposing total object count, object counts by type, relationship count, package count, and repository identity through the Public C API, so Studio's Dashboard and Repository Explorer never need to calculate these values themselves.

---

# Context

TASK-000023 (Engineering Object Enumeration API) is complete, built, and tested.

Per Work Package 012's explicit rule ("All new functionality shall be implemented through platform/api/oep_api.h. No internal Foundation headers shall be exposed."), both tasks were implemented against the same `platform/api/include/oep/api/oep_api.h` and `platform/api/src/oep_api.cpp` created in Work Package 011 — there is one Public C API, not one per task.

Statistics are computed entirely from `FoundationRuntime`'s existing accessors (`object_store()`, `relationship_store()`, `current_package_set()`, `current_metadata()`); no change to `FoundationRuntime`'s own interface was required or made.

---

# Objectives

Complete the following tasks only.

## Objective 1

Add `oep_repository_statistics_t` (fixed-layout, pointer-free) to `oep_api.h`: repository ID/name/version, total object count, object count by type, relationship count, package count.

---

## Objective 2

Add `oep_runtime_get_repository_statistics`, requiring an open repository, computed from existing `FoundationRuntime` accessors — no new `FoundationRuntime` surface.

---

## Objective 3

Ensure `package_count` and any other ambiguous count is unambiguously documented (e.g. distinguishing "every discovered package" from `oep_repository_status_t::loaded_package_count`, which counts only `Loaded` packages).

---

## Objective 4

Update `platform/api/README.md` (Repository Statistics section: usage, performance considerations) and `platform/runtime/README.md` (confirming no new Runtime surface was added).

---

## Objective 5

Add unit tests validating correct statistics for a populated repository (multiple object types, a relationship, no packages) and correct failure (`OEP_ERROR_INVALID_STATE`, zero-initialized output) without an open repository.

---

# Explicitly Out of Scope

Do not implement:

- Object or relationship mutation (create/update/delete) through the Public C API — enumeration and statistics are read-only, per the work package's objective
- Caching of statistics across calls
- Flutter/C#/Python/Java Bridge implementations
- Any change to `FoundationRuntime`'s own public interface

These systems belong to future tasks.

---

# Deliverables

- `oep_repository_statistics_t`, `oep_runtime_get_repository_statistics` (`platform/api`)
- Updated `platform/api/README.md`, `platform/runtime/README.md`
- Unit tests (`tests/api`)

---

# Acceptance Criteria

This task is complete only when:

- The project builds successfully.
- Repository statistics are available and correct through the Public C API.
- Studio-style consumers never need to calculate these values themselves.
- No internal Foundation header is exposed by the new functionality.
- The ABI remains stable (no existing structure's layout changed).
- Unit tests covering correct statistics and error handling pass.
- A full clean rebuild succeeds and the complete regression suite passes.

---

# Engineering Expectations

Favor readability.

Favor maintainability.

Favor simplicity.

No external dependency; statistics are computed from data `FoundationRuntime` already exposes.

Avoid speculative implementation (no caching, no mutation API, no Bridge implementation).

---

# Completion Checklist

Before marking this task complete, verify:

✓ Build succeeds

✓ Full regression suite passes

✓ Documentation updated (`platform/api/README.md`, `platform/runtime/README.md`)

✓ Project structure preserved

✓ Architecture unchanged

✓ Acceptance criteria satisfied

---

# After Completion

Update:

- PROJECT_STATUS.md
- CURRENT_SPRINT.md

Stop and await formal review of Work Package 012 (TASK-000023 + TASK-000024), per instructions.

---

# Verification Record

**Full clean rebuild:** `rm -rf build` followed by `cmake -S . -B build -G Ninja` and `cmake --build build` with MSVC 19.51 (Visual Studio Build Tools 18) — 99/99 build steps succeeded, producing every library (`oep_repository`, `oep_packages`, `oep_search`, `oep_validation`, `oep_exchange`, `oep_runtime`, `oep_api`, `oep_cli_core`), the `oep` executable, and all 23 test executables.

**Regression suite:** `ctest --output-on-failure` — 23/23 test suites passed (100%), including the expanded `oep_api_tests` suite (25 cases, up from 11) alongside all twenty-two pre-existing suites.

**Manual smoke tests:** `oep init smoke-repo` and `oep status smoke-repo` re-verified working, confirming Work Package 012 introduced no CLI regression (the CLI does not consume `platform/api` and was untouched by this work package).

**Architectural observations and decisions:**

- `ObjectStore::list_all()` has no defined enumeration order (plain filesystem directory iteration). The Public C API's `oep_object_store_list` therefore imposes its own deterministic sort (by `object_id`, ascending) before returning results, exactly mirroring the client-side sort `ObjectCommand` (the CLI's `object list`) has always applied. This is worth recording because "deterministic enumeration" (an explicit acceptance criterion) could not be satisfied by a naive pass-through of `list_all()` — the API boundary had to own the ordering guarantee itself, the same way the CLI already does, rather than assuming an underlying store provides one.
- `oep_object_store_list` is the only function in the entire Public C API (across Work Packages 011 and 012) that transfers heap-allocated memory to the caller. This was a deliberate, isolated exception to the API's otherwise uniform "no allocation, only fixed structures or static strings" design, made because a repository's object count is unbounded and cannot be represented in a fixed-size array the way `oep_repository_status_t`/`oep_object_info_t`/`oep_repository_statistics_t` represent their bounded data. The exception is contained: exactly one allocating function, paired with exactly one release function (`oep_object_list_release`), both documented at the point of allocation.
- Repository statistics intentionally duplicate no computation Studio would otherwise have to write itself — `total_object_count`, `object_count_by_type`, `relationship_count`, and `package_count` are all counted once, inside `oep_runtime_get_repository_statistics`, from data Foundation already has in memory via `FoundationRuntime`'s accessors. This directly satisfies the work package's "Studio shall not calculate these values itself" requirement rather than merely exposing raw enumeration and leaving the arithmetic to callers.
- `OEP_API_VERSION` was bumped 1 → 2 (additive functions/structures); `OEP_ABI_VERSION` remains 1 (no existing structure's layout changed) — consistent with the versioning contract established in Work Package 011's `platform/api/README.md`.
- `CLI_USAGE.md` was deliberately left unmodified: Work Package 012 restricts all new functionality to `platform/api/oep_api.h`, and no CLI command was added, changed, or now depends on the new API surface — updating a document with no applicable content would have been noise, not documentation.
