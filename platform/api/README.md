# API

Status: Foundation (Public C API + Bridge Support)

Purpose: The Public C API — Foundation's only supported native interface, per OEP-SPEC-021-PUBLIC_C_API — and the primitives external Bridge implementations (Flutter, C#, Python, Java, etc.) build on, per OEP-SPEC-022-FOUNDATION_BRIDGE_SUPPORT. Every Studio, SDK, and testing tool that isn't the CLI is expected to reach Foundation through this boundary rather than linking `platform/runtime` C++ types directly.

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

## Handle Ownership

- `oep_runtime_create` returns an owning handle; the caller must release it with exactly one call to `oep_runtime_destroy`. Returns `NULL` on invalid input or allocation failure — callers must check for `NULL` before use.
- `oep_runtime_destroy` closes an open repository first if necessary (mirroring `FoundationRuntime::shutdown`), then frees the handle. It is safe to call with `NULL` (a no-op) and safe to call on any state, including one with a repository still open.
- `oep_result_t` is a plain value type (a `struct` returned by value, containing only an `int`, two `enum`s, and a fixed `char[256]` buffer). It owns no heap memory and requires no release function.
- `oep_repository_status_t` is likewise a plain, pointer-free value type — safe to copy with `memcpy` and safe to convert directly into a language-native model by a Bridge.
- No function in this API returns a heap-allocated string or buffer the caller must free. Every string-valued output is either a pointer into static storage (documented as such at each function, e.g. `oep_foundation_version`, `oep_runtime_state_to_string`) or copied into a caller-supplied fixed buffer embedded in a result/status structure.

## Thread Safety

- A single `OEP_Runtime` handle is **not** safe for concurrent calls — exactly one thread may call functions against a given handle at a time, matching `FoundationRuntime`'s own single-threaded design (it introduces no internal locking).
- **Distinct** `OEP_Runtime` handles are fully independent and may be used concurrently from different threads, since each wraps its own `FoundationRuntime` instance with no shared mutable state.
- Functions that take no `OEP_Runtime` (`oep_foundation_version`, `oep_api_version`, `oep_abi_version`, `oep_runtime_state_to_string`, `oep_error_code_to_string`, `oep_error_category_to_string`) are stateless and safe to call from any thread at any time.
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
- `oep_api_version()` — `OEP_API_VERSION`, incremented for any addition or change to this API's functions or structures.
- `oep_abi_version()` — `OEP_ABI_VERSION`, incremented only when a change would break binary compatibility with a previously compiled caller (e.g. a struct layout change). Distinct from `OEP_API_VERSION` because a source-compatible addition need not be an ABI break.

Applications and Bridges should check `oep_abi_version()` against the version they were built against before relying on any struct layout in this header.

## Bridge Integration Guidance (OEP-SPEC-022)

A Bridge is any language-neutral adapter that exposes this API to a non-C++ host (Flutter/Dart, C#, Python, Java, etc.). Foundation provides the primitives every Bridge needs; it does not implement any Bridge itself:

- **Runtime state** — `oep_runtime_get_state`/`oep_runtime_state_to_string` give a Bridge a deterministic, five-value state machine to mirror in its own language-native model (e.g. a Dart enum or a C# enum), without needing to infer state from error text.
- **Error translation** — `error_code`/`error_category` are stable enums suitable for mapping onto language-native exception types or result unions; `error_message` is suitable for direct display or logging without further processing.
- **Data conversion** — `oep_repository_status_t` (and any future Bridge-facing structure this module adds) is a fixed-layout, pointer-free C struct: no `std::string`, no `std::vector`, no owning pointers. A Bridge can copy it directly into a native struct/record field-by-field without any Foundation-provided marshaling code, and doing so is deterministic — the same repository state always produces the same structure contents.
- **Compatibility** — a Bridge should check `oep_abi_version()` at startup and refuse to load (or warn loudly) if it was generated against/tested with a different ABI version, since a struct layout it depends on may have changed.

This module does not implement a Flutter, C#, Python, or Java Bridge — those are explicitly out of scope for both OEP-SPEC-021 and OEP-SPEC-022 and belong to future, separately ratified tasks.
