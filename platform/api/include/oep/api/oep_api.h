#ifndef OEP_API_H
#define OEP_API_H

/*
 * OEP Foundation Public C API
 * Per OEP-SPEC-021-PUBLIC_C_API, OEP-SPEC-022-FOUNDATION_BRIDGE_SUPPORT,
 * Work Package 012 (Engineering Object Enumeration, Repository
 * Statistics), Work Package 013 (Engineering Relationship Enumeration,
 * Repository Search), and Work Package 014 (Object Mutation,
 * Relationship Mutation, Transactions, Batch Mutation — the first
 * write-capable surface of this API).
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
#define OEP_API_VERSION 4

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

/* ------------------------------------------------------------------ */
/* Engineering Relationship Enumeration (Work Package 013, TASK-000025)*/
/* ------------------------------------------------------------------ */

typedef enum oep_relationship_type_t {
    OEP_RELATIONSHIP_TYPE_REFERENCES = 0,
    OEP_RELATIONSHIP_TYPE_CONTAINS = 1,
    OEP_RELATIONSHIP_TYPE_DEPENDS_ON = 2,
    OEP_RELATIONSHIP_TYPE_CONNECTED_TO = 3,
    OEP_RELATIONSHIP_TYPE_DOCUMENTS = 4,
    OEP_RELATIONSHIP_TYPE_IMPLEMENTS = 5,
} oep_relationship_type_t;

/* Returns a static, human-readable name for `type` (e.g. "Documents").
   Never freed by the caller. Deterministic. */
const char* oep_relationship_type_to_string(oep_relationship_type_t type);

#define OEP_MAX_RELATIONSHIP_ID 64
#define OEP_MAX_TIMESTAMP 32

/* A deterministic, fixed-layout snapshot of one Relationship's
   metadata — no pointers, no STL types, safe to copy by value or
   convert directly into a language-native model. String fields are
   truncated (never overflowed) if the underlying value is longer than
   the field's capacity. */
typedef struct oep_relationship_info_t {
    char relationship_id[OEP_MAX_RELATIONSHIP_ID];
    char source_object_id[OEP_MAX_OBJECT_ID];
    char target_object_id[OEP_MAX_OBJECT_ID];
    oep_relationship_type_t relationship_type;
    char author[OEP_MAX_OBJECT_AUTHOR];
    char description[OEP_MAX_OBJECT_DESCRIPTION];
    char created_utc[OEP_MAX_TIMESTAMP];
} oep_relationship_info_t;

/* Returns the number of Relationships in the currently open
   repository. Only valid from RepositoryOpen; fails with
   OEP_ERROR_INVALID_STATE otherwise, in which case `*out_count` is 0.
   `out_count` must not be NULL. */
oep_result_t oep_relationship_store_get_count(OEP_Runtime runtime, int* out_count);

/* Populates `out_relationship` with the Relationship identified by
   `relationship_id`. Only valid from RepositoryOpen; fails with
   OEP_ERROR_INVALID_STATE if no repository is open, or
   OEP_ERROR_NOT_FOUND if no relationship with that ID exists, in
   which case `*out_relationship` is zero-initialized.
   `relationship_id` and `out_relationship` must not be NULL. */
oep_result_t oep_relationship_store_get_by_id(OEP_Runtime runtime, const char* relationship_id,
                                               oep_relationship_info_t* out_relationship);

/* An enumerated collection of Relationships. `items` is a
   Foundation-owned heap array of `count` elements, sorted
   deterministically by relationship_id (ascending, byte-wise) — the
   same order every time for unchanged repository contents. A list
   that was never successfully populated has `items == NULL` and
   `count == 0`, and is always safe to pass to
   oep_relationship_list_release. Follows the same ownership model as
   oep_object_list_t (Work Package 012, TASK-000023). */
typedef struct oep_relationship_list_t {
    oep_relationship_info_t* items;
    int count;
} oep_relationship_list_t;

/* Enumerates every Relationship in the currently open repository into
   `out_list`, sorted deterministically by relationship_id. Only valid
   from RepositoryOpen; fails with OEP_ERROR_INVALID_STATE otherwise,
   in which case `*out_list` is zero-initialized. `out_list` must not
   be NULL.

   Ownership: on success, the caller owns `out_list->items` and must
   release it with exactly one call to oep_relationship_list_release.
   Do not call `free`/`delete` on `items` directly. */
oep_result_t oep_relationship_store_list(OEP_Runtime runtime, oep_relationship_list_t* out_list);

/* Releases the heap array owned by `list` (if any) and zeroes
   `list->items`/`list->count`. Safe to call on a zero-initialized or
   already-released list (a no-op). `list` itself may be NULL (a
   no-op). */
void oep_relationship_list_release(oep_relationship_list_t* list);

/* ------------------------------------------------------------------ */
/* Repository Search (Work Package 013, TASK-000026)                   */
/* ------------------------------------------------------------------ */

/* The field a query matched against, mirroring
   oep::search::MatchLocation (OEP-SPEC-006-REPOSITORY_SEARCH). */
typedef enum oep_match_location_t {
    OEP_MATCH_LOCATION_NAME = 0,
    OEP_MATCH_LOCATION_DESCRIPTION = 1,
    OEP_MATCH_LOCATION_AUTHOR = 2,
    OEP_MATCH_LOCATION_TAGS = 3,
    OEP_MATCH_LOCATION_OBJECT_TYPE = 4,
    OEP_MATCH_LOCATION_RELATIONSHIP_TYPE = 5,
} oep_match_location_t;

/* Returns a static, human-readable name for `location`. Never freed by
   the caller. Deterministic. */
const char* oep_match_location_to_string(oep_match_location_t location);

/* One Engineering Object search hit — a fixed-layout projection of
   oep::search::ObjectSearchResult. */
typedef struct oep_object_search_result_t {
    char object_id[OEP_MAX_OBJECT_ID];
    oep_object_type_t object_type;
    char display_name[OEP_MAX_OBJECT_NAME];
    oep_match_location_t match_location;
    double match_score;
} oep_object_search_result_t;

/* items is a Foundation-owned heap array, in exactly the order
   Foundation's Search Engine produced it — the Public API never
   reorders search results. Follows the same ownership model as
   oep_object_list_t. */
typedef struct oep_object_search_result_list_t {
    oep_object_search_result_t* items;
    int count;
} oep_object_search_result_list_t;

/* One Relationship search hit — a fixed-layout projection of
   oep::search::RelationshipSearchResult. */
typedef struct oep_relationship_search_result_t {
    char relationship_id[OEP_MAX_RELATIONSHIP_ID];
    char source_object_id[OEP_MAX_OBJECT_ID];
    char target_object_id[OEP_MAX_OBJECT_ID];
    oep_relationship_type_t relationship_type;
    oep_match_location_t match_location;
    double match_score;
} oep_relationship_search_result_t;

/* Same ownership model as oep_object_search_result_list_t. */
typedef struct oep_relationship_search_result_list_t {
    oep_relationship_search_result_t* items;
    int count;
} oep_relationship_search_result_list_t;

/* The combined result of a repository-wide search: every matching
   object and every matching relationship, each in its own list and in
   its own Search-Engine-produced order (the two lists are never
   merged or interleaved, matching `oep search`'s own "Objects: ... /
   Relationships: ..." presentation). */
typedef struct oep_repository_search_result_t {
    oep_object_search_result_list_t objects;
    oep_relationship_search_result_list_t relationships;
} oep_repository_search_result_t;

/* Searches both Engineering Objects and Relationships for `query`
   (case-insensitive, partial-match, per oep::search::SearchEngine).
   Only valid from RepositoryOpen; fails with OEP_ERROR_INVALID_STATE
   otherwise. Fails with OEP_ERROR_INVALID_ARGUMENT for a NULL or empty
   `query`. On any failure, `*out_result` is zero-initialized.
   `query` and `out_result` must not be NULL.

   Ownership: on success, the caller owns both
   out_result->objects.items and out_result->relationships.items and
   must release them with exactly one call to
   oep_repository_search_result_release. */
oep_result_t oep_search_repository(OEP_Runtime runtime, const char* query,
                                    oep_repository_search_result_t* out_result);

/* Releases both lists owned by `result` (if any) and zeroes them.
   Safe to call on a zero-initialized or already-released result (a
   no-op). `result` itself may be NULL (a no-op). */
void oep_repository_search_result_release(oep_repository_search_result_t* result);

/* Searches Engineering Objects only for `query`. Same state/argument
   rules as oep_search_repository. Ownership: release with
   oep_object_search_result_list_release. */
oep_result_t oep_search_objects(OEP_Runtime runtime, const char* query, oep_object_search_result_list_t* out_list);

/* Releases the heap array owned by `list` (if any) and zeroes it.
   Safe to call on a zero-initialized or already-released list, and
   safe to call with `list == NULL` (both no-ops). */
void oep_object_search_result_list_release(oep_object_search_result_list_t* list);

/* Searches Relationships only for `query`. Same state/argument rules
   as oep_search_repository. Ownership: release with
   oep_relationship_search_result_list_release. */
oep_result_t oep_search_relationships(OEP_Runtime runtime, const char* query,
                                      oep_relationship_search_result_list_t* out_list);

/* Releases the heap array owned by `list` (if any) and zeroes it.
   Safe to call on a zero-initialized or already-released list, and
   safe to call with `list == NULL` (both no-ops). */
void oep_relationship_search_result_list_release(oep_relationship_search_result_list_t* list);

/* ------------------------------------------------------------------ */
/* Object Mutation (Work Package 014, TASK-000027)                     */
/* ------------------------------------------------------------------ */
/*
 * Every mutation function below delegates entirely to
 * FoundationRuntime, which delegates entirely to ObjectStore/
 * RelationshipStore — the Public API never manipulates a store
 * directly and never re-implements a validation rule Foundation
 * already enforces. In particular, `name`/`source_object_id`/
 * `target_object_id` are only checked here for a NULL pointer (an ABI
 * safety requirement); an empty or otherwise invalid value is left to
 * fail Foundation's own validation, so the same rule is enforced in
 * exactly one place.
 *
 * If a transaction is active (see "Transactions" below) and a
 * mutation fails, the active transaction is automatically rolled back
 * and deactivated before the failure is returned.
 */

/* Creates a new Engineering Object. `name` must not be NULL (an empty
   name still fails, via Foundation's own validation, with
   OEP_ERROR_INVALID_ARGUMENT). `description`/`author` may be NULL,
   treated as empty. `tags` may be NULL iff `tag_count` is 0;
   otherwise `tags` must point to `tag_count` NUL-terminated strings
   (individual entries may not be NULL). Unlike oep_object_info_t's
   fixed-size `tags` field, there is no cap on how many tags may be
   supplied here — a tag list longer than OEP_MAX_OBJECT_TAGS is
   accepted and stored in full; only reading it back out through a
   fixed oep_object_info_t truncates it.

   Only valid from RepositoryOpen; fails with OEP_ERROR_INVALID_STATE
   otherwise. On success, `out_object` (if non-NULL) is populated with
   the created object, including its generated object_id. */
oep_result_t oep_object_create(OEP_Runtime runtime, oep_object_type_t object_type, const char* name,
                                const char* description, const char* author, const char* const* tags, int tag_count,
                                oep_object_info_t* out_object);

/* Replaces name/description/author/tags on the object identified by
   `object_id`. object_id, object_type, and the object's created
   timestamp are never changed — matching ObjectStore::update's own
   contract exactly, since object_type is immutable throughout
   Foundation's Engineering Object model. Fails with
   OEP_ERROR_NOT_FOUND if no object with `object_id` exists. Argument
   rules for name/description/author/tags/tag_count are the same as
   oep_object_create. */
oep_result_t oep_object_update(OEP_Runtime runtime, const char* object_id, const char* name, const char* description,
                                const char* author, const char* const* tags, int tag_count,
                                oep_object_info_t* out_object);

/* Deletes the Engineering Object identified by `object_id`. Fails
   with OEP_ERROR_NOT_FOUND if no such object exists. Does not cascade
   to Relationships referencing the deleted object — this mirrors
   ObjectStore::remove's own existing behavior and is not a new
   restriction introduced by this API. */
oep_result_t oep_object_delete(OEP_Runtime runtime, const char* object_id);

/* ------------------------------------------------------------------ */
/* Relationship Mutation (Work Package 014, TASK-000028)               */
/* ------------------------------------------------------------------ */

/* Creates a new Relationship between two existing Engineering Objects.
   `source_object_id`/`target_object_id` must not be NULL. Fails with
   OEP_ERROR_NOT_FOUND if either referenced object does not exist, or
   OEP_ERROR_INVALID_ARGUMENT if the relationship is otherwise invalid
   (e.g. source equals target) — both checks are performed by
   RelationshipStore::create itself, not duplicated here.
   `author`/`description` may be NULL, treated as empty. Only valid
   from RepositoryOpen. On success, `out_relationship` (if non-NULL)
   is populated with the created relationship. */
oep_result_t oep_relationship_create(OEP_Runtime runtime, const char* source_object_id,
                                      const char* target_object_id, oep_relationship_type_t relationship_type,
                                      const char* author, const char* description,
                                      oep_relationship_info_t* out_relationship);

/* Replaces author/description on the relationship identified by
   `relationship_id`. relationship_id, source_object_id,
   target_object_id, relationship_type, and the relationship's created
   timestamp are never changed — matching RelationshipStore::update's
   own contract exactly. Fails with OEP_ERROR_NOT_FOUND if no
   relationship with `relationship_id` exists. */
oep_result_t oep_relationship_update(OEP_Runtime runtime, const char* relationship_id, const char* author,
                                      const char* description, oep_relationship_info_t* out_relationship);

/* Deletes the Relationship identified by `relationship_id`. Fails
   with OEP_ERROR_NOT_FOUND if no such relationship exists. */
oep_result_t oep_relationship_delete(OEP_Runtime runtime, const char* relationship_id);

/* ------------------------------------------------------------------ */
/* Transactions (Work Package 014, TASK-000029)                        */
/* ------------------------------------------------------------------ */
/*
 * A transaction groups object/relationship mutations into one
 * deterministically reversible unit. Only one transaction may be
 * active per OEP_Runtime handle at a time — nested
 * oep_transaction_begin() calls fail with OEP_ERROR_INVALID_STATE
 * rather than silently stacking.
 *
 * Each mutation still writes to the repository immediately when
 * called (Foundation's stores have no staged/uncommitted write
 * concept); while a transaction is active, Foundation additionally
 * records what each successful mutation would need to undo it.
 * oep_transaction_commit() discards that record (nothing further to
 * do — every mutation already persisted). oep_transaction_rollback()
 * replays the record in reverse, undoing each mutation via the same
 * stores a normal mutation would use.
 *
 * If any object or relationship mutation function above fails while a
 * transaction is active, Foundation automatically rolls back and
 * deactivates the transaction before returning the failure — the
 * caller does not need to (but safely may) call
 * oep_transaction_rollback() afterward.
 *
 * Closing the repository or shutting down the Runtime while a
 * transaction is active automatically rolls it back first.
 */

/* Begins a new transaction. Only valid from RepositoryOpen; fails
   with OEP_ERROR_INVALID_STATE if no repository is open or a
   transaction is already active. */
oep_result_t oep_transaction_begin(OEP_Runtime runtime);

/* Commits the active transaction (discarding its undo record; every
   mutation within it is already persisted). Fails with
   OEP_ERROR_INVALID_STATE if no transaction is currently active. */
oep_result_t oep_transaction_commit(OEP_Runtime runtime);

/* Rolls back the active transaction, undoing every mutation performed
   since oep_transaction_begin() in reverse order. Fails with
   OEP_ERROR_INVALID_STATE if no transaction is currently active. */
oep_result_t oep_transaction_rollback(OEP_Runtime runtime);

/* Returns nonzero iff a transaction is currently active on `runtime`.
   Never fails; returns 0 if `runtime` is NULL. */
int oep_transaction_is_active(OEP_Runtime runtime);

/* ------------------------------------------------------------------ */
/* Batch Mutation (Work Package 014, TASK-000030)                      */
/* ------------------------------------------------------------------ */
/*
 * Batch mutation is a convenience over the transaction primitives
 * above: every spec is created in array order (the deterministic
 * execution order this section requires), inside a transaction. If no
 * transaction is active when the batch function is called, one is
 * begun and committed automatically around the whole batch; if the
 * caller already has a transaction active, the batch participates in
 * it. Any failure rolls back the complete batch (and, if the caller
 * already had a transaction active going in, everything else in that
 * transaction too — the transaction, not the batch call, is the unit
 * of rollback).
 *
 * `specs`/`created` hold `const char*` fields, not fixed buffers:
 * these are input-only structures the caller populates and Foundation
 * only reads for the duration of the call — no ownership transfer,
 * and still no STL/C++ types cross the boundary.
 */

/* One object to create as part of a batch. `name` must not be NULL;
   `description`/`author` may be NULL (treated as empty); `tags` may
   be NULL iff `tag_count` is 0. The caller retains ownership of every
   pointer; Foundation only reads them for the duration of the
   oep_batch_create_objects call. */
typedef struct oep_object_create_spec_t {
    oep_object_type_t object_type;
    const char* name;
    const char* description;
    const char* author;
    const char* const* tags;
    int tag_count;
} oep_object_create_spec_t;

/* `created` is in execution order (the same order as the input
   `specs` array) — it is deliberately NOT sorted by object_id the way
   oep_object_store_list is, since preserving input/execution order is
   exactly the determinism guarantee this section requires. On
   failure, `created` is always empty (items = NULL, count = 0): the
   whole batch was rolled back. */
typedef struct oep_batch_create_objects_result_t {
    int success;
    /* -1 on full success; otherwise the 0-based index into the input
       `specs` array of the item that failed. */
    int failed_index;
    oep_object_list_t created;
} oep_batch_create_objects_result_t;

/* Creates every object in `specs` (an array of `count` specs) in
   order, inside a transaction (see "Batch Mutation" above for the
   begin/commit/rollback semantics). Only valid from RepositoryOpen.
   `specs` must not be NULL if `count` > 0. `out_result` must not be
   NULL.

   Ownership: on success, the caller owns `out_result->created.items`
   and must release it with exactly one call to
   oep_batch_create_objects_result_release. */
oep_result_t oep_batch_create_objects(OEP_Runtime runtime, const oep_object_create_spec_t* specs, int count,
                                       oep_batch_create_objects_result_t* out_result);

/* Releases the heap array owned by `result->created` (if any) and
   zeroes it. Safe to call on a zero-initialized or already-released
   result, and safe to call with `result == NULL` (both no-ops). */
void oep_batch_create_objects_result_release(oep_batch_create_objects_result_t* result);

/* One relationship to create as part of a batch.
   `source_object_id`/`target_object_id` must not be NULL;
   `author`/`description` may be NULL (treated as empty). The caller
   retains ownership of every pointer. */
typedef struct oep_relationship_create_spec_t {
    const char* source_object_id;
    const char* target_object_id;
    oep_relationship_type_t relationship_type;
    const char* author;
    const char* description;
} oep_relationship_create_spec_t;

/* Same execution-order and rollback-on-failure contract as
   oep_batch_create_objects_result_t. */
typedef struct oep_batch_create_relationships_result_t {
    int success;
    int failed_index;
    oep_relationship_list_t created;
} oep_batch_create_relationships_result_t;

/* Creates every relationship in `specs` (an array of `count` specs)
   in order, inside a transaction. Same semantics as
   oep_batch_create_objects. Ownership: release with
   oep_batch_create_relationships_result_release. */
oep_result_t oep_batch_create_relationships(OEP_Runtime runtime, const oep_relationship_create_spec_t* specs,
                                             int count, oep_batch_create_relationships_result_t* out_result);

/* Releases the heap array owned by `result->created` (if any) and
   zeroes it. Safe to call on a zero-initialized or already-released
   result, and safe to call with `result == NULL` (both no-ops). */
void oep_batch_create_relationships_result_release(oep_batch_create_relationships_result_t* result);

#ifdef __cplusplus
}
#endif

#endif /* OEP_API_H */
