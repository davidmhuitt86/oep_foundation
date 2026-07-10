#ifndef OEP_API_H
#define OEP_API_H

/*
 * OEP Foundation Public C API
 * Per OEP-SPEC-021-PUBLIC_C_API, OEP-SPEC-022-FOUNDATION_BRIDGE_SUPPORT,
 * and Work Package 012 (Engineering Object Enumeration, Repository
 * Statistics).
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
#define OEP_API_VERSION 2

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

/* ------------------------------------------------------------------ */
/* Engineering Object Enumeration (Work Package 012, TASK-000023)      */
/* ------------------------------------------------------------------ */

typedef enum oep_object_type_t {
    OEP_OBJECT_TYPE_DOCUMENT = 0,
    OEP_OBJECT_TYPE_DIAGRAM = 1,
    OEP_OBJECT_TYPE_COMPONENT = 2,
    OEP_OBJECT_TYPE_PROCEDURE = 3,
    OEP_OBJECT_TYPE_PROJECT = 4,
    OEP_OBJECT_TYPE_IMAGE = 5,
} oep_object_type_t;

/* The number of distinct oep_object_type_t values. Also the size of
   oep_repository_statistics_t::object_count_by_type. */
#define OEP_OBJECT_TYPE_COUNT 6

/* Returns a static, human-readable name for `type` (e.g. "Component").
   Never freed by the caller. Deterministic. */
const char* oep_object_type_to_string(oep_object_type_t type);

#define OEP_MAX_OBJECT_ID 64
#define OEP_MAX_OBJECT_NAME 256
#define OEP_MAX_OBJECT_AUTHOR 128
#define OEP_MAX_OBJECT_VERSION 32
#define OEP_MAX_OBJECT_DESCRIPTION 1024
#define OEP_MAX_OBJECT_TAGS 16
#define OEP_MAX_TAG_LENGTH 64

/* A deterministic, fixed-layout snapshot of one Engineering Object's
   metadata — no pointers, no STL types, safe to copy by value or
   convert directly into a language-native model. String fields are
   truncated (never overflowed) if the underlying value is longer than
   the field's capacity; `tag_count` is capped at OEP_MAX_OBJECT_TAGS
   (additional tags beyond the cap are simply not included). */
typedef struct oep_object_info_t {
    char object_id[OEP_MAX_OBJECT_ID];
    oep_object_type_t object_type;
    char name[OEP_MAX_OBJECT_NAME];
    char author[OEP_MAX_OBJECT_AUTHOR];
    char version[OEP_MAX_OBJECT_VERSION];
    char description[OEP_MAX_OBJECT_DESCRIPTION];
    int tag_count;
    char tags[OEP_MAX_OBJECT_TAGS][OEP_MAX_TAG_LENGTH];
} oep_object_info_t;

/* Returns the number of Engineering Objects in the currently open
   repository. Only valid from RepositoryOpen; fails with
   OEP_ERROR_INVALID_STATE otherwise, in which case `*out_count` is 0.
   `out_count` must not be NULL. */
oep_result_t oep_object_store_get_count(OEP_Runtime runtime, int* out_count);

/* Populates `out_object` with the Engineering Object identified by
   `object_id`. Only valid from RepositoryOpen; fails with
   OEP_ERROR_INVALID_STATE if no repository is open, or
   OEP_ERROR_NOT_FOUND if no object with that ID exists, in which case
   `*out_object` is zero-initialized. `object_id` and `out_object` must
   not be NULL. */
oep_result_t oep_object_store_get_by_id(OEP_Runtime runtime, const char* object_id, oep_object_info_t* out_object);

/* An enumerated collection of Engineering Objects. `items` is a
   Foundation-owned heap array of `count` elements, sorted
   deterministically by object_id (ascending, byte-wise) — the same
   order every time for unchanged repository contents. A list that was
   never successfully populated (e.g. the owning oep_result_t reported
   failure) has `items == NULL` and `count == 0`, and is always safe to
   pass to oep_object_list_release. */
typedef struct oep_object_list_t {
    oep_object_info_t* items;
    int count;
} oep_object_list_t;

/* Enumerates every Engineering Object in the currently open repository
   into `out_list`, sorted deterministically by object_id. Only valid
   from RepositoryOpen; fails with OEP_ERROR_INVALID_STATE otherwise,
   in which case `*out_list` is zero-initialized (items = NULL,
   count = 0). `out_list` must not be NULL.

   Ownership: on success, the caller owns `out_list->items` and must
   release it with exactly one call to oep_object_list_release. Do not
   call `free`/`delete` on `items` directly — it was allocated by
   Foundation and must be released through the matching Foundation
   function. */
oep_result_t oep_object_store_list(OEP_Runtime runtime, oep_object_list_t* out_list);

/* Releases the heap array owned by `list` (if any) and zeroes
   `list->items`/`list->count`. Safe to call on a zero-initialized or
   already-released list (a no-op). `list` itself may be NULL (a
   no-op). */
void oep_object_list_release(oep_object_list_t* list);

/* ------------------------------------------------------------------ */
/* Repository Statistics (Work Package 012, TASK-000024)               */
/* ------------------------------------------------------------------ */

/* A deterministic, fixed-layout snapshot of repository-wide counts,
   computed by Foundation so Studio (or any other API consumer) never
   has to enumerate and count objects/relationships/packages itself.
   No pointers; safe to copy by value or convert directly into a
   language-native model. */
typedef struct oep_repository_statistics_t {
    char repository_id[64];
    char repository_name[256];
    char repository_version[32];
    int total_object_count;
    /* Indexed by oep_object_type_t; object_count_by_type[OEP_OBJECT_TYPE_COMPONENT]
       is the number of Component objects, and so on. */
    int object_count_by_type[OEP_OBJECT_TYPE_COUNT];
    int relationship_count;
    /* Every discovered package, regardless of Loaded/Invalid/Disabled
       state — distinct from oep_repository_status_t::loaded_package_count,
       which counts only Loaded packages. */
    int package_count;
} oep_repository_statistics_t;

/* Populates `out_statistics` from the currently open repository. Only
   valid from RepositoryOpen; fails with OEP_ERROR_INVALID_STATE
   otherwise, in which case `*out_statistics` is zero-initialized.
   `out_statistics` must not be NULL. */
oep_result_t oep_runtime_get_repository_statistics(OEP_Runtime runtime,
                                                    oep_repository_statistics_t* out_statistics);

#ifdef __cplusplus
}
#endif

#endif /* OEP_API_H */
