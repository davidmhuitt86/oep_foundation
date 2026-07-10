# TASK
# TASK.md
## Open Engineering Platform (OEP)

Task ID: 000026

Status: Complete

---

# Current Task

Implement the Repository Search Public API per Work Package 013 / TASK-000026 (second of two tasks; follows TASK-000025/Engineering Relationship Enumeration API immediately, per instruction to complete both before stopping).

This delivers `oep_search_repository`, `oep_search_objects`, and `oep_search_relationships`, exposing Foundation's `SearchEngine` through the Public C API so OEP Studio's Search Workspace can consume Foundation search results directly and never implement repository search independently.

---

# Context

TASK-000025 (Engineering Relationship Enumeration API) is complete, built, and tested.

Per Work Package 013's rule ("All new functionality shall be implemented only through platform/api/oep_api.h"), both tasks extend the same `oep_api.h`/`oep_api.cpp` created in Work Package 011 and extended in Work Package 012 — there remains exactly one Public C API.

Search results must never be reordered by the API boundary — `SearchEngine::search_objects`/`search_relationships` already sort deterministically (descending match score, then index order); the C API performs a one-to-one field conversion into fixed C structures and nothing more. Author/tag/type filtering is explicitly out of scope for this API per the work package — it remains a Studio-side responsibility, exactly as the CLI's `--type`/`--author`/`--tag` flags already apply after `SearchEngine` returns.

---

# Objectives

Complete the following tasks only.

## Objective 1

Add `oep_match_location_t`, `oep_object_search_result_t`, `oep_relationship_search_result_t` (all fixed-layout, pointer-free) to `oep_api.h`.

---

## Objective 2

Add `oep_search_objects` and `oep_search_relationships`, each a thin projection of the corresponding `SearchEngine` method, allocating one heap array released via a dedicated `*_list_release` function.

---

## Objective 3

Add `oep_search_repository`, returning two independent lists (objects and relationships) rather than a single merged/reordered list, preserving each sub-list's Search-Engine-produced order exactly.

---

## Objective 4

Ensure a `NULL`/empty query fails with `OEP_ERROR_INVALID_ARGUMENT`, distinct from a valid-but-non-matching query (which succeeds with a zero-length result list).

---

## Objective 5

Update `platform/api/README.md` (Repository Search section: usage, ownership, thread safety, performance) and `platform/runtime/README.md` (confirming search reuses the existing `search_engine()` accessor, no new Runtime surface).

---

## Objective 6

Add unit tests validating object-only, relationship-only, and combined search, correct match score/location/type fields, empty-query rejection, error handling without an open repository, and re-run `oep search` manually to confirm no CLI regression.

---

# Explicitly Out of Scope

Do not implement:

- Server-side filtering by author, tag, or object type (Studio's responsibility after results are returned)
- Any reordering, re-ranking, or merging of object and relationship search results
- Relationship or object mutation through the Public C API
- Any change to `FoundationRuntime`'s or `SearchEngine`'s own public interface

These systems belong to future tasks.

---

# Deliverables

- `oep_match_location_t`, `oep_object_search_result_t`, `oep_relationship_search_result_t`, `oep_repository_search_result_t`
- `oep_search_repository`, `oep_search_objects`, `oep_search_relationships` and their release functions (`platform/api`)
- Updated `platform/api/README.md`, `platform/runtime/README.md`
- Unit tests (`tests/api`)

---

# Acceptance Criteria

This task is complete only when:

- The project builds successfully.
- Repository search, object-only search, and relationship-only search are all available and correct through the Public C API.
- Result ordering exactly matches `SearchEngine`'s own output; the API never reorders results.
- No internal Foundation header is exposed by the new functionality.
- The ABI remains stable (no existing structure's layout changed).
- Unit tests covering all three search forms, match fields, and error handling pass.
- `oep search` (CLI) produces unchanged output, confirming no regression.
- A full clean rebuild succeeds and the complete regression suite passes.

---

# Engineering Expectations

Favor readability.

Favor maintainability.

Favor simplicity.

No external dependency; search results are read directly from the existing `SearchEngine` instance.

Avoid speculative implementation (no filtering, no caching, no cross-type ranking).

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

Stop and await formal review of Work Package 013 (TASK-000025 + TASK-000026), per instructions.

---

# Verification Record

**Full clean rebuild:** `rm -rf build` followed by `cmake -S . -B build -G Ninja` and `cmake --build build` with MSVC 19.51 (Visual Studio Build Tools 18) — 99/99 build steps succeeded, producing every library (`oep_repository`, `oep_packages`, `oep_search`, `oep_validation`, `oep_exchange`, `oep_runtime`, `oep_api`, `oep_cli_core`), the `oep` executable, and all 23 test executables.

**Regression suite:** `ctest --output-on-failure` — 23/23 test suites passed (100%), including the expanded `oep_api_tests` suite (grown further in this work package) alongside all twenty-two pre-existing suites.

**Manual smoke tests:** `oep init`, `oep object create` ×2, `oep relationship create`/`list`, and `oep search`/`oep search objects`/`oep search relationships` re-run against a real generated repository — output unchanged from before this work package, confirming no CLI regression (per the work package's explicit "Re-run oep relationship / oep search to confirm no regressions" instruction).

**Architectural observations and decisions:**

- `oep_search_repository` intentionally returns two independent, unmerged lists rather than one combined/ranked list. Producing a single cross-type ranking would have required Foundation to invent an ordering between an object hit and a relationship hit that `SearchEngine` itself never computes — directly violating "the Public API shall never reorder search results" (the ordering the API would be inventing wouldn't be a reordering of an existing order, it would be a *new* order with no Foundation-side source of truth). Keeping the two lists separate sidesteps the question entirely and mirrors what `oep search`'s combined form has always done (two separately-headed sections, not one interleaved list).
- Both `oep_search_objects` and `oep_search_relationships` fail with `OEP_ERROR_INVALID_ARGUMENT` for an empty query, because `SearchEngine::search_objects`/`search_relationships` themselves treat an empty query as a structural error (not "zero matches"). The C API preserves this distinction deliberately — a Bridge should not have to guess whether a zero-length result list means "nothing matched" or "the query was invalid."
- One implementation-time correction is worth recording: internal helper functions (`validate_search_arguments`, `build_object_search_result_list`, `build_relationship_search_result_list`) were initially written as an anonymous namespace nested inside the `extern "C" { ... }` block. Although this likely would have compiled, nesting a namespace inside a linkage-specification block is unusual and its interaction with declared linkage is not something to leave ambiguous in code meant to model a stable, portable C ABI — the block was moved to file scope, immediately before `extern "C"` opens, alongside the file's other internal helpers (`to_capi_state`, `zero_status`, etc.), before the first build attempt.
- Relationship enumeration's sort-by-`relationship_id` requirement is the same finding already made in Work Package 012 for object enumeration, now confirmed to generalize: `RelationshipStore::list_all()`, like `ObjectStore::list_all()`, has no defined order (plain filesystem iteration), so "deterministic enumeration" is an API-boundary guarantee, not a store-level one, for every enumerable collection Foundation exposes.
- `CLI_USAGE.md` was deliberately left unmodified, per the work package's own rule that CLI documentation is only updated when user-visible CLI behavior changes — it does not, in this work package.
