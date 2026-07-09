# TASK
# TASK.md
## Open Engineering Platform (OEP)

Task ID: 000022

Status: Complete

---

# Current Task

Implement Foundation Bridge Support per OEP-SPEC-022-FOUNDATION_BRIDGE_SUPPORT (Work Package 011, second of two tasks; follows TASK-000021/Public C API immediately, per instruction to complete both before stopping).

This extends the Public C API created in TASK-000021 with the primitives external Bridge implementations (Flutter, C#, Python, Java, etc.) need: deterministic runtime state reporting, error category classification, and Bridge-compatible (fixed-layout, pointer-free) data structures.

---

# Context

TASK-000021 (Public C API) is complete, built, and tested.

Both specs describe one coherent ABI surface — OEP-SPEC-021 establishes the C API's lifecycle/handles/error/versioning shape, and OEP-SPEC-022 asks for specific additions (state stringification, error category, Bridge-safe structures) to that same surface — so both tasks were implemented together against the same `platform/api/include/oep/api/oep_api.h` and `platform/api/src/oep_api.cpp`, per the work package's pre-authorization for shared refactoring between the two tasks. Neither spec's acceptance criteria required a second, separate header or library.

`platform/api` was previously an unspecified placeholder module (`Status: Placeholder`); this work package gives it its first real implementation.

---

# Objectives

Complete the following tasks only.

## Objective 1

Create `platform/api` as a pure C ABI (`oep_api.h`) wrapping `FoundationRuntime`: opaque `OEP_Runtime` handle, runtime/repository lifecycle functions, `oep_result_t` (success/error code/error message), and versioning (Foundation version, API version, ABI version).

---

## Objective 2

Extend the same ABI with Bridge-facing primitives: `oep_error_category_t` (coarse error grouping), deterministic stringification functions (`oep_runtime_state_to_string`/`oep_error_code_to_string`/`oep_error_category_to_string`), and a fixed-layout `oep_repository_status_t` structure with no pointers or STL types.

---

## Objective 3

Ensure no native C++ exception ever crosses the API boundary — every exported function catches all exceptions and translates them into `OEP_ERROR_INTERNAL`.

---

## Objective 4

Write `platform/api/README.md` documenting API lifecycle, handle ownership, thread safety, error handling, versioning, and Bridge integration guidance (satisfying both specs' identical documentation requirement in one file).

---

## Objective 5

Add unit tests validating lifecycle, errors (including `NULL`-argument handling), ownership (destroy safety, destroying with an open repository), version reporting, state transitions, and error/category reporting.

---

# Explicitly Out of Scope

Do not implement:

- Flutter bindings, or a C#/Python/Java Bridge implementation
- Network APIs or remote Foundation
- Any change to `FoundationRuntime`'s own public interface (the C API wraps it unchanged)
- Runtime, SDK, Studios, Networking, Authentication, Plugin system, GUI

These systems belong to future tasks.

---

# Deliverables

- `platform/api/include/oep/api/oep_api.h` (public ABI)
- `platform/api/src/oep_api_internal.hpp` / `oep_api.cpp` (implementation)
- `platform/api/README.md`
- `oep::runtime::kFoundationVersion` (shared version constant, consolidating the previously CLI-private literal)
- Unit tests (`tests/api`)

---

# Acceptance Criteria

This task is complete only when:

- The project builds successfully.
- Runtime and repository lifecycle functions operate correctly through the C API.
- The ABI is pure C; no C++ implementation types or STL containers cross the boundary.
- No native exception crosses the API boundary under any tested failure condition.
- Runtime state, error code, and error category are all exposed and deterministic.
- `platform/api/README.md` is complete (lifecycle, ownership, thread safety, error handling, versioning, Bridge guidance).
- Unit tests covering lifecycle, errors, ownership, version reporting, state transitions, and structure compatibility pass.
- A full clean rebuild succeeds and the complete regression suite passes.

---

# Engineering Expectations

Favor readability.

Favor maintainability.

Favor simplicity.

No external dependency; the C API is implemented in C++ against the existing `oep_runtime` library.

Avoid speculative implementation (no Flutter/C#/Python/Java Bridge, no network transport).

---

# Completion Checklist

Before marking this task complete, verify:

✓ Build succeeds

✓ Full regression suite passes

✓ Documentation updated (`platform/api/README.md`)

✓ Project structure preserved

✓ Architecture unchanged

✓ Acceptance criteria satisfied

---

# After Completion

Update:

- PROJECT_STATUS.md
- CURRENT_SPRINT.md

Stop and await formal review of Work Package 011 (TASK-000021 + TASK-000022), per instructions.

---

# Verification Record

**Full clean rebuild:** `rm -rf build` followed by `cmake -S . -B build -G Ninja` and `cmake --build build` with MSVC 19.51 (Visual Studio Build Tools 18) — 99/99 build steps succeeded, producing every library (`oep_repository`, `oep_packages`, `oep_search`, `oep_validation`, `oep_exchange`, `oep_runtime`, `oep_api`, `oep_cli_core`), the `oep` executable, and all 23 test executables.

**Regression suite:** `ctest --output-on-failure` — 23/23 test suites passed (100%), including the new `oep_api_tests` suite alongside all twenty-two pre-existing suites.

**Manual smoke tests:** `oep init smoke-repo` and `oep status smoke-repo` re-verified working after the `kFoundationVersion` consolidation refactor, confirming no regression to the CLI's version reporting.

**Architectural observations and decisions:**

- `platform/api` is a static library (`oep_api`) linked **privately** against `oep_runtime` — its public include directory exposes only `oep_api.h`. A consumer that links `oep_api` gets no transitive access to `platform/runtime`'s C++ headers, enforcing "never expose C++ classes" at the build-system level, not just by header discipline.
- The concrete type behind `OEP_Runtime` (`struct oep_runtime_impl`, wrapping a `FoundationRuntime` by value) is defined only in the private `src/oep_api_internal.hpp`, never installed. Applications only ever hold a `struct oep_runtime_impl*` they cannot dereference without that private header — matching the spec's "applications shall never inspect handle contents."
- Error classification (`oep_error_code_t`/`oep_error_category_t`) is synthesized at the API boundary, not sourced from `FoundationRuntime`, because `RuntimeResult` only carries a success flag and a free-text error string. The boundary layer knows which state preconditions it checked itself (yielding `OEP_ERROR_INVALID_STATE` deterministically, checked *before* delegating to `FoundationRuntime`) and pattern-matches a small number of known failure-message substrings (e.g. "could not load repository metadata" → `OEP_ERROR_NOT_FOUND`) for the rest. This is a pragmatic, documented choice rather than plumbing a new error-code concept through `FoundationRuntime` itself, which the spec does not require and which would have expanded scope into Runtime's own interface.
- Both specs' identical instruction to update `platform/api/README.md` was satisfied with one document rather than two, since splitting Public-C-API documentation from Bridge-support documentation would have meant either duplicating the lifecycle/ownership sections or forcing a reader to cross-reference two files for what is, in the shipped implementation, a single ABI.
- The `oep::runtime::kFoundationVersion` consolidation (moving the literal out of `platform/cli/src/foundation_version.hpp` into a new public `platform/runtime/include/oep/runtime/foundation_version.hpp`) was the one cross-cutting refactor performed, authorized by the work package. `oep::cli::kFoundationVersion` is unchanged as a name and value — only its definition site moved — so no existing public interface changed.
