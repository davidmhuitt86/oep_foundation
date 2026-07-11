# TASK
# TASK.md
## Open Engineering Platform (OEP)

Task ID: 000030

Status: Complete

---

# Current Task

Implement Public Batch Mutation per Work Package 014 / TASK-000030 (fourth of four tasks; follows TASK-000027/Object Mutation, TASK-000028/Relationship Mutation, and TASK-000029/Transactions, per instruction to complete the whole work package before stopping).

This delivers `oep_batch_create_objects`/`oep_batch_create_relationships`, allowing multiple object or relationship creations to execute as one deterministic, all-or-nothing unit through the Public C API — the first write-capable surface Foundation has ever exposed externally.

---

# Context

TASK-000027, TASK-000028, and TASK-000029 are complete, built, and tested.

Before implementation, the existing Foundation architecture, `FoundationRuntime`'s current interface, and the Public C API (Work Packages 011–013) were reviewed per the work package's explicit instruction. The review found one real gap, not a conflict: `FoundationRuntime` exposed `object_store()`/`relationship_store()` as read-oriented accessors (returning `const ObjectStore*`/`const RelationshipStore*`) but had no mutation entry points of its own. This is exactly the gap the work package's architectural guidance anticipated ("every Public C API mutation function shall delegate through Foundation Runtime," "stores shall never become directly accessible through the Public API") — filling it with new `FoundationRuntime` methods, rather than having the Public API reach `ObjectStore`/`RelationshipStore` directly, was the correct and intended resolution, not an independent architectural solution requiring review. No other conflict, ambiguity, or missing capability was found; implementation proceeded without stopping for review.

Batch mutation is built entirely on the transaction primitives from TASK-000029 (`begin_transaction`/`commit_transaction`/`rollback_transaction`) rather than a second rollback mechanism — every spec is created in array order inside a transaction, which is begun and committed automatically if the caller had none active, or which the batch simply participates in if the caller already had one active.

---

# Objectives

Complete the following tasks only.

## Objective 1

Add `oep_object_create_spec_t`/`oep_relationship_create_spec_t` (input-only, `const char*`-based, no ownership transfer) and `oep_batch_create_objects_result_t`/`oep_batch_create_relationships_result_t` (output, reusing the same allocate-once/release-once ownership model as `oep_object_list_t`/`oep_relationship_list_t`) to `oep_api.h`.

---

## Objective 2

Add `oep_batch_create_objects`/`oep_batch_create_relationships`, backed by new `FoundationRuntime::batch_create_objects`/`batch_create_relationships` methods that loop over the transaction primitives in array order.

---

## Objective 3

Ensure execution order is deterministic (array order = creation order = the order reported back in `created`) and that any failure rolls back the complete batch — and, if the caller already had a transaction active, everything else in that transaction too, since the transaction is the true unit of rollback.

---

## Objective 4

Update `platform/api/README.md`, `platform/runtime/README.md`, and (new) `platform/api/MUTATION_API.md` covering object mutation, relationship mutation, transactions, and batch mutation together, per the work package's consolidated documentation requirement.

---

## Objective 5

Write a standalone C API test program (`platform/api/manual_test/capi_manual_test.c`, compiled as plain C) exercising every mutation, transaction, and batch scenario the work package's Verification section lists, and add corresponding automated unit tests to `tests/api/oep_api_tests.cpp`.

---

# Explicitly Out of Scope

Do not implement:

- Partial-patch update semantics (update remains a full-record replace of the mutable fields, matching `ObjectStore::update`/`RelationshipStore::update`'s own existing contract)
- Bulk update or bulk delete (only bulk *create*, per the spec's explicit "Batch Object Create, Batch Relationship Create")
- Nested transactions
- Any change to `FoundationRuntime`'s existing read-only Service Registry accessors
- Any Studio-side change
- Any CLI-visible change (`oep relationship`/`oep search`/etc. behavior is unchanged)

These systems belong to future tasks or are explicitly excluded by the work package.

---

# Deliverables

- `oep_object_create`/`oep_object_update`/`oep_object_delete` (TASK-000027)
- `oep_relationship_create`/`oep_relationship_update`/`oep_relationship_delete` (TASK-000028)
- `oep_transaction_begin`/`_commit`/`_rollback`/`_is_active` (TASK-000029)
- `oep_batch_create_objects`/`oep_batch_create_relationships` (TASK-000030)
- `FoundationRuntime::create_object`/`update_object`/`delete_object`, `create_relationship`/`update_relationship`/`delete_relationship`, `begin_transaction`/`commit_transaction`/`rollback_transaction`/`transaction_active`, `batch_create_objects`/`batch_create_relationships`
- `platform/api/manual_test/capi_manual_test.c` (new C-language build target)
- `platform/api/MUTATION_API.md` (new)
- Updated `platform/api/README.md`, `platform/runtime/README.md`
- Expanded `tests/api/oep_api_tests.cpp`

---

# Acceptance Criteria

This task is complete only when:

- The project builds successfully, including the new plain-C manual test target.
- Object creation, update, and deletion all function through the Public C API.
- Relationship creation, update, and deletion all function through the Public C API.
- Transaction rollback (explicit and automatic-on-failure) is deterministic and correctly reversible.
- Batch mutation preserves execution order and rolls back completely on any failure.
- No store is directly reachable through the Public API; every mutation delegates through `FoundationRuntime`.
- No native exception crosses the API boundary under any tested failure condition.
- `platform/api/README.md`, `platform/runtime/README.md`, and `platform/api/MUTATION_API.md` are complete.
- A full clean rebuild succeeds and the complete regression suite passes.
- The standalone C manual verification program passes every check.
- Existing CLI commands (`oep relationship`, `oep search`, and others) show no regression.

---

# Engineering Expectations

Favor readability.

Favor maintainability.

Favor simplicity.

No external dependency; mutation, transactions, and batch mutation are all built from `ObjectStore`/`RelationshipStore` methods that already existed before this work package.

Avoid speculative implementation (no partial-patch updates, no bulk update/delete, no nested transactions, no cross-repository transactions).

---

# Completion Checklist

Before marking this task complete, verify:

✓ Build succeeds (including the new C-language manual test target)

✓ Full regression suite passes

✓ Standalone C API manual verification program passes

✓ Documentation updated (`platform/api/README.md`, `platform/runtime/README.md`, `platform/api/MUTATION_API.md`)

✓ Project structure preserved

✓ Architecture unchanged (no store directly reachable through the Public API; Runtime remains the sole orchestration layer)

✓ Acceptance criteria satisfied

---

# After Completion

Update:

- PROJECT_STATUS.md
- CURRENT_SPRINT.md

Stop and await formal review of Work Package 014 (TASK-000027 + TASK-000028 + TASK-000029 + TASK-000030), per instructions.

---

# Verification Record

**Full clean rebuild:** `rm -rf build` followed by `cmake -S . -B build -G Ninja` and `cmake --build build` with MSVC 19.51 (Visual Studio Build Tools 18), `LANGUAGES CXX C` (C enabled for this work package's manual test program) — 101/101 build steps succeeded, producing every library (`oep_repository`, `oep_packages`, `oep_search`, `oep_validation`, `oep_exchange`, `oep_runtime`, `oep_api`, `oep_cli_core`), the `oep` executable, the new `oep_capi_manual_test` executable, and all 23 CTest-registered test executables.

**Regression suite:** `ctest --output-on-failure` — 23/23 test suites passed (100%), including the substantially expanded `oep_api_tests` suite.

**Manual verification (standalone C API test program):** `platform/api/manual_test/capi_manual_test.c`, compiled as plain C, run against a freshly-built repository fixture — **54/54 checks passed**, covering object creation/update/deletion, relationship creation/update/deletion, explicit transaction rollback, automatic rollback on a mid-transaction failure, and batch mutation (both full success with execution-order preservation and rollback-on-failure with correct `failed_index`).

**CLI regression check:** manually re-ran `oep init`, `oep object create`/`list`, `oep relationship create`/`list`, `oep search`/`oep search objects`/`oep search relationships`, `oep batch create`, `oep validate`, `oep export`, `oep version`, and `oep --help` against a real generated repository — output unchanged from prior work packages.

**Architectural observations and decisions:**

- **No architectural conflict required stopping for review.** The pre-implementation review (explicitly required by this work package) found `FoundationRuntime` had no mutation surface at all — this was recognized as the gap the work package's own architectural guidance instructed how to fill (new Runtime methods, never direct store access from the API), not an ambiguity or conflict. This is recorded here per the work package's instruction to document architectural observations even when none blocked progress.
- **Transactions are an in-memory undo log, not a second persistence mechanism.** `ObjectStore`/`RelationshipStore` gained no staged/uncommitted-write concept; every mutation still writes to disk immediately, transaction or not. A transaction only adds bookkeeping: while active, `FoundationRuntime` records what would undo each successful mutation, and `rollback_transaction` replays that log in reverse using the stores' own pre-existing `remove`/`update`/`restore` methods. This keeps determinism trivial (the undo path is exactly the forward path, run backward with recorded snapshots) and required no change to the Repository layer at all.
- **`restore()` is reused, not reinvented, for rollback.** Undoing a delete calls `ObjectStore::restore`/`RelationshipStore::restore` — the exact mechanism Import and Templates (Work Package 009) already established for writing a record back with its original identity intact. This is a direct instance of "reuse existing Runtime/ObjectStore/RelationshipStore functionality whenever possible."
- **Automatic rollback on any mutation failure inside an active transaction** is a strict, literal reading of "Failures shall: Abort transaction, Roll back changes" — a mutation failure (even one unrelated to the earlier successful mutations in the same transaction, e.g. an unrelated validation error) tears down the whole transaction rather than leaving it open for the caller to decide. This was a deliberate choice to keep transaction state always unambiguous (never partially-failed-but-still-open) rather than a softer "log and continue" alternative the spec's wording does not support.
- **Batch mutation is sugar over transactions, not a parallel mechanism.** `batch_create_objects`/`batch_create_relationships` contain no rollback logic of their own — they call `create_object`/`create_relationship` in a loop, and rely entirely on the automatic-rollback-on-failure behavior above. A consequence, documented in `MUTATION_API.md`: if a batch is called while the caller already has a transaction active, a batch failure rolls back the caller's entire transaction, not just the batch's own entries — the transaction, not the batch call, is the true unit of rollback, and this is intentional rather than an oversight.
- **The standalone C manual test program is a genuine architectural verification, not just a formality.** Compiling `capi_manual_test.c` as plain C (`LANGUAGES CXX C` added to the top-level `CMakeLists.txt`) and linking it against `oep_api` (implemented in C++) proves the ABI surface really is C-callable, not merely C-styled in a way only a C++ translation unit could actually consume — a meaningfully stronger check than a C++ test file including a C-compatible header.
- Two implementation-time compile errors were caught and fixed by the build itself: (1) two `foundation_runtime.cpp` return statements returned a store-level result type where the enclosing method's own `RuntimeResult` was expected (same shape, different type — C++ has no implicit conversion between them); (2) a literal `*/` occurring inside prose inside a block comment in `oep_api.h` (`oep_object_*/oep_relationship_*`) terminated the comment early, corrupting the rest of the header — reworded to avoid the character sequence. Both are noted since they're easy mistakes to repeat in future work packages that add prose-heavy comments referencing multiple `oep_*` function name prefixes.
