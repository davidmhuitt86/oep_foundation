#ifndef OEP_API_H
#define OEP_API_H

/*
 * OEP Foundation Public C API
 * Per OEP-SPEC-021-PUBLIC_C_API and OEP-SPEC-022-FOUNDATION_BRIDGE_SUPPORT.
 *
 * This is the only supported native interface into Foundation. It is a
 * pure C ABI: no C++ classes, no STL types, and no exceptions ever cross
 * this boundary. Every function that can fail returns an oep_result_t;
 * native exceptions raised internally are caught at the boundary and
 * translated into OEP_ERROR_INTERNAL results, never propagated.
 *
 * See platform/api/README.md for the full lifecycle, ownership, thread
 * safety, and Bridge integration guidance.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Versioning (OEP-SPEC-021 section 8)                                 */
/* ------------------------------------------------------------------ */

/* The Public C API's own version. Incremented whenever a function or
   structure is added, changed, or removed. */
#define OEP_API_VERSION 1

/* The ABI version. Incremented only when a change would break binary
   compatibility with a previously compiled caller (e.g. a struct
   layout change). Distinct from OEP_API_VERSION, which may advance for
   additions that remain ABI-compatible. */
#define OEP_ABI_VERSION 1

/* Returns the Foundation version this build implements (e.g. "0.1.0"),
   the same version reported by `oep status`/`oep version`. The
   returned pointer references static storage and must not be freed. */
const char* oep_foundation_version(void);

/* Returns OEP_API_VERSION. */
int oep_api_version(void);

/* Returns OEP_ABI_VERSION. */
int oep_abi_version(void);

/* ------------------------------------------------------------------ */
/* Opaque handles (OEP-SPEC-021 section 4)                             */
/* ------------------------------------------------------------------ */

/* Applications shall never inspect handle contents; a handle is only
   ever passed back into this API. */
typedef struct oep_runtime_impl* OEP_Runtime;

/* ------------------------------------------------------------------ */
/* Runtime state (OEP-SPEC-022 section 3)                              */
/* ------------------------------------------------------------------ */

typedef enum oep_runtime_state_t {
    OEP_STATE_UNINITIALIZED = 0,
    OEP_STATE_INITIALIZED = 1,
    OEP_STATE_REPOSITORY_OPEN = 2,
    OEP_STATE_REPOSITORY_CLOSED = 3,
    OEP_STATE_SHUTDOWN = 4,
} oep_runtime_state_t;

/* Returns a static, human-readable name for `state` (e.g.
   "RepositoryOpen"). The returned pointer references static storage
   and must not be freed. Deterministic: the same input always
   produces the same output. */
const char* oep_runtime_state_to_string(oep_runtime_state_t state);

/* ------------------------------------------------------------------ */
/* Error reporting (OEP-SPEC-021 section 6, OEP-SPEC-022 section 4)    */
/* ------------------------------------------------------------------ */

typedef enum oep_error_code_t {
    OEP_ERROR_NONE = 0,
    OEP_ERROR_INVALID_ARGUMENT = 1,
    OEP_ERROR_INVALID_STATE = 2,
    OEP_ERROR_NOT_FOUND = 3,
    OEP_ERROR_OPERATION_FAILED = 4,
    OEP_ERROR_INTERNAL = 5,
} oep_error_code_t;

/* A coarse classification of `oep_error_code_t`, so a Bridge can group
   errors (e.g. to decide whether to surface a retry option) without
   needing to enumerate every individual error code. */
typedef enum oep_error_category_t {
    OEP_ERROR_CATEGORY_NONE = 0,
    OEP_ERROR_CATEGORY_VALIDATION = 1, /* bad input: OEP_ERROR_INVALID_ARGUMENT */
    OEP_ERROR_CATEGORY_STATE = 2,      /* wrong lifecycle state: OEP_ERROR_INVALID_STATE */
    OEP_ERROR_CATEGORY_IO = 3,         /* filesystem/repository access: OEP_ERROR_NOT_FOUND, OEP_ERROR_OPERATION_FAILED */
    OEP_ERROR_CATEGORY_INTERNAL = 4,   /* unexpected internal failure: OEP_ERROR_INTERNAL */
} oep_error_category_t;

/* Returns a static, human-readable name for `code`. Never freed by the
   caller. */
const char* oep_error_code_to_string(oep_error_code_t code);

/* Returns a static, human-readable name for `category`. Never freed by
   the caller. */
const char* oep_error_category_to_string(oep_error_category_t category);

#define OEP_MAX_ERROR_MESSAGE 256

/* Every API function that can fail returns an oep_result_t by value.
   `success` is nonzero on success, in which case error_code is
   OEP_ERROR_NONE, error_category is OEP_ERROR_CATEGORY_NONE, and
   error_message is empty. On failure, error_message is a
   NUL-terminated, human-readable, English-language description
   suitable for logging or for a Bridge to translate/present; it is
   truncated (never overflowed) if longer than
   OEP_MAX_ERROR_MESSAGE - 1 bytes. No heap allocation is associated
   with an oep_result_t: it is a plain value type and requires no
   release function. */
typedef struct oep_result_t {
    int success;
    oep_error_code_t error_code;
    oep_error_category_t error_category;
    char error_message[OEP_MAX_ERROR_MESSAGE];
} oep_result_t;

/* ------------------------------------------------------------------ */
/* Runtime lifecycle (OEP-SPEC-021 section 5)                          */
/* ------------------------------------------------------------------ */

/* Creates a new, Uninitialized Runtime handle representing the given
   Foundation version (NUL-terminated UTF-8; copied internally, so the
   caller's buffer need not outlive the call). Returns NULL if
   `foundation_version` is NULL or allocation fails.

   Ownership: the caller owns the returned handle and must release it
   with oep_runtime_destroy exactly once. */
OEP_Runtime oep_runtime_create(const char* foundation_version);

/* Releases a Runtime handle. If the Runtime has an open repository,
   it is closed first (mirroring FoundationRuntime::shutdown). Safe to
   call with NULL (a no-op). After this call, `runtime` must not be
   used again. */
void oep_runtime_destroy(OEP_Runtime runtime);

/* Transitions the Runtime from Uninitialized to Initialized. Fails
   with OEP_ERROR_INVALID_STATE if not currently Uninitialized. */
oep_result_t oep_runtime_initialize(OEP_Runtime runtime);

/* Opens the repository rooted at `repository_path` (NUL-terminated
   UTF-8). Only valid from Initialized or RepositoryClosed; fails with
   OEP_ERROR_INVALID_STATE otherwise. Fails with OEP_ERROR_NOT_FOUND or
   OEP_ERROR_OPERATION_FAILED if the repository cannot be opened (e.g.
   missing or corrupt repository.json). */
oep_result_t oep_runtime_open_repository(OEP_Runtime runtime, const char* repository_path);

/* Closes the currently open repository. Only valid from
   RepositoryOpen; fails with OEP_ERROR_INVALID_STATE otherwise. */
oep_result_t oep_runtime_close_repository(OEP_Runtime runtime);

/* Transitions the Runtime to Shutdown, closing an open repository
   first if necessary. Idempotent: calling this from Shutdown again
   fails with OEP_ERROR_INVALID_STATE rather than crashing. */
oep_result_t oep_runtime_shutdown(OEP_Runtime runtime);

/* Returns the Runtime's current state. Never fails; returns
   OEP_STATE_UNINITIALIZED if `runtime` is NULL. */
oep_runtime_state_t oep_runtime_get_state(OEP_Runtime runtime);

/* ------------------------------------------------------------------ */
/* Repository status (OEP-SPEC-021 section 5, OEP-SPEC-022 section 5)  */
/* ------------------------------------------------------------------ */

/* A deterministic, fixed-layout snapshot of the currently open
   repository, safe to copy across the API boundary and convert into a
   language-native model by a Bridge (OEP-SPEC-022 section 5). Contains
   no pointers, so it may be copied by value with memcpy. */
typedef struct oep_repository_status_t {
    /* Nonzero iff a repository is currently open; every other field is
       only meaningful when this is nonzero. */
    int repository_open;
    char repository_id[64];
    char repository_name[256];
    char repository_version[32];
    int loaded_package_count;
} oep_repository_status_t;

/* Populates `out_status` from the currently open repository. Only
   valid from RepositoryOpen; fails with OEP_ERROR_INVALID_STATE
   otherwise, in which case `*out_status` is zero-initialized.
   `out_status` must not be NULL. */
oep_result_t oep_runtime_get_repository_status(OEP_Runtime runtime, oep_repository_status_t* out_status);

#ifdef __cplusplus
}
#endif

#endif /* OEP_API_H */
