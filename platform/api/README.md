# API

Status: Foundation (Public C API + Bridge Support + Repository Content Exposure)

Purpose: The Public C API — Foundation's only supported native interface, per OEP-SPEC-021-PUBLIC_C_API — the primitives external Bridge implementations (Flutter, C#, Python, Java, etc.) build on, per OEP-SPEC-022-FOUNDATION_BRIDGE_SUPPORT, and (per Work Package 012) the Engineering Object enumeration and repository statistics surface OEP Studio and other consumers need without ever touching Foundation internals. Every Studio, SDK, and testing tool that isn't the CLI is expected to reach Foundation through this boundary rather than linking `platform/runtime` C++ types directly.

## Architecture

- `include/oep/api/oep_api.h` — the entire public ABI: a pure C header (`extern "C"`), no C++ classes, no STL types, no exceptions
- `src/oep_api_internal.hpp` — private implementation details (the concrete type behind `OEP_Runtime`, error-result helpers); never installed or exposed
- `src/oep_api.cpp` — implementation, wrapping `oep::runtime::FoundationRuntime`; every exported function catches all exceptions at the boundary and translates them into an `OEP_ERROR_INTERNAL` result rather than letting them propagate

Built as a static library, `oep_api`, linked privately against `oep_runtime` — consumers of `oep_api` only ever see `oep_api.h`; they never need (and cannot easily obtain) `platform/runtime`'s C++ headers.

## API Lifecycle

```c
OEP_Runtime runtime = oep_runtime_create("0.1.0");   /* Uninitialized */
oep_runtime_initialize(runtime);                      /* -> Initialized */
oep_runtime_open_repository(runtime, "/path/to/repo"); /* -> RepositoryOpen */

oep_repository_status_t status;
oep_runtime_get_repository_status(runtime, &status);

oep_runtime_close_repository(runtime);                /* -> RepositoryClosed */
oep_runtime_shutdown(runtime);                         /* -> Shutdown */
oep_runtime_destroy(runtime);
```

State transitions are deterministic and mirror `FoundationRuntime` exactly (per OEP-SPEC-011-FOUNDATION_RUNTIME, re-exposed per OEP-SPEC-022 section 3):

```text
Uninitialized -> Initialized -> RepositoryOpen -> RepositoryClosed -> Shutdown
                                       |__________________________________^
                                        (RepositoryClosed -> RepositoryOpen again is allowed)
```

Calling a lifecycle function from the wrong state never crashes or corrupts state — it returns a failed `oep_result_t` with `error_code == OEP_ERROR_INVALID_STATE`, and the Runtime's state is unchanged.

## Engineering Object Enumeration (Work Package 012, TASK-000023)

While a repository is open, Engineering Objects can be counted, enumerated, and looked up by ID without touching `platform/repository`'s C++ types:

```c
int count = 0;
oep_object_store_get_count(runtime, &count);

oep_object_info_t widget;
oep_object_store_get_by_id(runtime, "044ba21d-85b2-4502-a5ea-3787fec41367", &widget);

oep_object_list_t all_objects;
oep_object_store_list(runtime, &all_objects);
for (int i = 0; i < all_objects.count; ++i) {
    printf("%s\t%s\n", all_objects.items[i].object_id, all_objects.items[i].name);
}
oep_object_list_release(&all_objects); /* required after a successful oep_object_store_list */
```

`oep_object_info_t` is a fixed-layout, pointer-free structure (`object_id`, `object_type`, `name`, `author`, `version`, `description`, and up to `OEP_MAX_OBJECT_TAGS` tags) — every string field is a fixed-size buffer, truncated rather than overflowed if the underlying value is longer. `oep_object_store_list` always returns objects sorted deterministically by `object_id` (ascending, byte-wise), so repeated enumeration of an unchanged repository always produces the same order — Bridges and Studio should rely on this instead of re-sorting client-side.

**Memory ownership:** `oep_object_store_get_count` and `oep_object_store_get_by_id` allocate nothing — the former writes an `int`, the latter fills a caller-supplied `oep_object_info_t`. `oep_object_store_list`, however, allocates a heap array (`oep_object_list_t::items`) sized to the repository's object count; on success, the caller owns that array and **must** release it with exactly one call to `oep_object_list_release`, never with `free`/`delete` directly (it was allocated with `new[]` on the Foundation side of the ABI boundary, and only Foundation's own release function is guaranteed to match the allocator that produced it). A list that was never successfully populated (`items == NULL`, `count == 0`) is always safe to pass to `oep_object_list_release` — releasing it is a no-op, not an error.

**Performance:** `oep_object_store_list` performs a full directory enumeration and one heap allocation sized to the object count every call — it is not cached. For a single object, prefer `oep_object_store_get_by_id` over listing everything and searching client-side.

## Repository Statistics (Work Package 012, TASK-000024)

```c
oep_repository_statistics_t statistics;
oep_runtime_get_repository_statistics(runtime, &statistics);

printf("%d objects, %d relationships, %d packages\n",
       statistics.total_object_count, statistics.relationship_count, statistics.package_count);
printf("%d Components\n", statistics.object_count_by_type[OEP_OBJECT_TYPE_COMPONENT]);
```

`oep_repository_statistics_t` is a fixed-layout structure carrying repository identity (`repository_id`/`repository_name`/`repository_version`), `total_object_count`, `object_count_by_type` (indexed by `oep_object_type_t`), `relationship_count`, and `package_count` (every discovered package, regardless of Loaded/Invalid/Disabled state — distinct from `oep_repository_status_t::loaded_package_count`, which counts only `Loaded` packages). Every count is computed by Foundation from the currently open repository; Studio (or any other consumer) must never recompute these values itself by enumerating objects/relationships/packages client-side — doing so would duplicate logic this API already provides deterministically. No allocation is involved; `oep_runtime_get_repository_statistics` fills a caller-supplied structure, same as `oep_runtime_get_repository_status`.

**Performance:** computing statistics requires one full enumeration each of objects, relationships, and packages — proportional to repository size, not cached across calls. Callers that need statistics repeatedly (e.g. a Dashboard polling for updates) should poll at a sensible interval rather than every frame.

## Handle Ownership

- `oep_runtime_create` returns an owning handle; the caller must release it with exactly one call to `oep_runtime_destroy`. Returns `NULL` on invalid input or allocation failure — callers must check for `NULL` before use.
- `oep_runtime_destroy` closes an open repository first if necessary (mirroring `FoundationRuntime::shutdown`), then frees the handle. It is safe to call with `NULL` (a no-op) and safe to call on any state, including one with a repository still open.
- `oep_result_t` is a plain value type (a `struct` returned by value, containing only an `int`, two `enum`s, and a fixed `char[256]` buffer). It owns no heap memory and requires no release function.
- `oep_repository_status_t`, `oep_object_info_t`, and `oep_repository_statistics_t` are likewise plain, pointer-free value types — safe to copy with `memcpy` and safe to convert directly into a language-native model by a Bridge.
- `oep_object_list_t` is the one exception: `items` is a Foundation-owned heap array. It is always paired with `oep_object_list_release`, the API's only release function — see "Engineering Object Enumeration" above for the exact ownership contract.
- Aside from `oep_object_list_t::items`, no function in this API returns a heap-allocated string or buffer the caller must free. Every other string-valued output is either a pointer into static storage (documented as such at each function, e.g. `oep_foundation_version`, `oep_runtime_state_to_string`) or copied into a caller-supplied fixed buffer embedded in a result/status structure.

## Thread Safety

- A single `OEP_Runtime` handle is **not** safe for concurrent calls — exactly one thread may call functions against a given handle at a time, matching `FoundationRuntime`'s own single-threaded design (it introduces no internal locking).
- **Distinct** `OEP_Runtime` handles are fully independent and may be used concurrently from different threads, since each wraps its own `FoundationRuntime` instance with no shared mutable state.
- Functions that take no `OEP_Runtime` (`oep_foundation_version`, `oep_api_version`, `oep_abi_version`, `oep_runtime_state_to_string`, `oep_error_code_to_string`, `oep_error_category_to_string`, `oep_object_type_to_string`) are stateless and safe to call from any thread at any time.
- `oep_object_list_release` operates only on the `oep_object_list_t` passed to it (freeing its `items` array) and touches no `OEP_Runtime` state — safe to call from any thread, including one different from the thread that populated the list, as long as no other thread is concurrently using or releasing the same list.
- Bridge implementations that expose Foundation to a multi-threaded host environment (e.g. a UI event loop plus background workers) must serialize access to a single handle themselves — a mutex or a single dedicated "Foundation thread" per handle are both correct strategies.

## Error Handling

Every function that can fail returns an `oep_result_t`:

```c
typedef struct oep_result_t {
    int success;
    oep_error_code_t error_code;
    oep_error_category_t error_category;
    char error_message[OEP_MAX_ERROR_MESSAGE];
} oep_result_t;
```

`error_code` is one of `OEP_ERROR_NONE` (only on success), `OEP_ERROR_INVALID_ARGUMENT`, `OEP_ERROR_INVALID_STATE`, `OEP_ERROR_NOT_FOUND`, `OEP_ERROR_OPERATION_FAILED`, or `OEP_ERROR_INTERNAL`. `error_category` is a coarser grouping (`OEP_ERROR_CATEGORY_VALIDATION`/`STATE`/`IO`/`INTERNAL`) — per OEP-SPEC-022 section 4, this lets a Bridge branch on a small, stable set of categories without needing to keep its own logic in sync with every individual error code Foundation might add later. `oep_error_code_to_string`/`oep_error_category_to_string` provide stable, English-language names for logging; `error_message` itself is meant for a human (or for a Bridge to pass through to one) — Bridges should switch on `error_code`/`error_category`, not parse `error_message`.

Native C++ exceptions never cross this boundary: every exported function wraps its body in `try`/`catch (...)`, translating any unexpected exception into `OEP_ERROR_INTERNAL` with the exception's `what()` (if available) as the message.

## Versioning

Three independent version signals are exposed, per OEP-SPEC-021 section 8:

- `oep_foundation_version()` — the Foundation version this build implements (e.g. `"0.1.0"`), shared with the CLI (`oep::runtime::kFoundationVersion`, `oep version`/`oep status`) so both layers always agree.
- `oep_api_version()` — `OEP_API_VERSION`, incremented for any addition or change to this API's functions or structures. Currently `2`, bumped by Work Package 012's purely additive Engineering Object Enumeration and Repository Statistics functions.
- `oep_abi_version()` — `OEP_ABI_VERSION`, incremented only when a change would break binary compatibility with a previously compiled caller (e.g. a struct layout change). Distinct from `OEP_API_VERSION` because a source-compatible addition need not be an ABI break. Still `1` — Work Package 012 added new functions and structures but did not change the layout of any existing one.

Applications and Bridges should check `oep_abi_version()` against the version they were built against before relying on any struct layout in this header.

## Bridge Integration Guidance (OEP-SPEC-022)

A Bridge is any language-neutral adapter that exposes this API to a non-C++ host (Flutter/Dart, C#, Python, Java, etc.). Foundation provides the primitives every Bridge needs; it does not implement any Bridge itself:

- **Runtime state** — `oep_runtime_get_state`/`oep_runtime_state_to_string` give a Bridge a deterministic, five-value state machine to mirror in its own language-native model (e.g. a Dart enum or a C# enum), without needing to infer state from error text.
- **Error translation** — `error_code`/`error_category` are stable enums suitable for mapping onto language-native exception types or result unions; `error_message` is suitable for direct display or logging without further processing.
- **Data conversion** — `oep_repository_status_t`, `oep_object_info_t`, and `oep_repository_statistics_t` are all fixed-layout, pointer-free C structs: no `std::string`, no `std::vector`, no owning pointers. A Bridge can copy any of them directly into a native struct/record field-by-field without any Foundation-provided marshaling code, and doing so is deterministic — the same repository state always produces the same structure contents. `oep_object_list_t` is the sole exception carrying an owned pointer (`items`); a Bridge should convert each `oep_object_info_t` in the array into its native collection type and then call `oep_object_list_release` promptly, rather than holding the array open indefinitely.
- **Compatibility** — a Bridge should check `oep_abi_version()` at startup and refuse to load (or warn loudly) if it was generated against/tested with a different ABI version, since a struct layout it depends on may have changed.

This module does not implement a Flutter, C#, Python, or Java Bridge — those are explicitly out of scope for both OEP-SPEC-021 and OEP-SPEC-022 and belong to future, separately ratified tasks. Work Package 012 similarly does not add object/relationship mutation (create/update/delete) to this API — only enumeration and read-only statistics, per its stated objective of satisfying OEP Studio's read requirements without exposing Foundation internals.
