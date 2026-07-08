# TASK
# TASK.md
## Open Engineering Platform (OEP)

Task ID: 000006

Status: Complete

---

# Current Task

Implement the Repository Search System per OEP-SPEC-006-REPOSITORY_SEARCH.

This delivers a `SearchEngine` in the `platform/search` module, providing deterministic, offline, case-insensitive, partial-match search across Engineering Objects and Relationships.

---

# Context

TASK-000005 (Engineering Object Relationship Model) is complete and accepted.

OEP-SPEC-006-REPOSITORY_SEARCH.md defines an in-memory (non-persistent) search index rebuilt on load, searchable fields, result shape, and the `BuildIndex`/`SearchObjects`/`SearchRelationships`/`ClearIndex` interface. Semantic search, AI-assisted search, and distributed search are explicitly out of scope.

`platform/search` already exists as a scaffolded, empty module per the frozen architecture (OEP-ARCH-001) — this task specifies and fills it in for the first time.

---

# Objectives

Complete the following tasks only.

## Objective 1

Add enumeration support to `platform/repository`'s stores (`ObjectStore::list_all`, `RelationshipStore::list_all`) so an index can be built from everything stored in a repository. Refactor `RelationshipStore::list_by_object` to reuse this rather than duplicating the enumeration.

---

## Objective 2

Implement `SearchEngine` in `platform/search`: `build_index`, `clear_index`, `search_objects`, `search_relationships`.

---

## Objective 3

Support case-insensitive, partial-match search over: Engineering Objects (name, description, author, tags, object type) and Relationships (relationship type, description, author). Results are deterministic and repeatable for the same repository state and query.

---

## Objective 4

Each object result includes object ID, object type, display name, match location, and match score. Each relationship result additionally identifies source object, target object, and relationship type.

---

## Objective 5

Add unit tests validating indexing, searching (including case-insensitivity, partial match, and each searchable field), rebuilding the index, empty-repository behavior, and invalid-query handling.

---

# Explicitly Out of Scope

Do not implement:

- Semantic search
- AI-assisted search
- Registry search
- Distributed search
- Persistent/incremental indexing
- Runtime, SDK, Studios, Exchange, Networking, Authentication, Plugin system, GUI

These systems belong to future tasks.

---

# Deliverables

- `platform/search` module (`SearchEngine`, result types)
- `ObjectStore::list_all`, `RelationshipStore::list_all`
- Unit tests

---

# Acceptance Criteria

This task is complete only when:

- The project builds successfully.
- Repository indexes are built successfully from an `ObjectStore`/`RelationshipStore`.
- Engineering Objects and Relationships are both searchable per the field list above.
- Search results are deterministic across repeated runs.
- Unit tests covering indexing, searching, rebuilding, and invalid-query handling pass.

---

# Engineering Expectations

Favor readability.

Favor maintainability.

Favor simplicity.

The Search Engine must remain independent of the CLI and Studios, and must never modify repository contents.

Avoid speculative implementation (no persistence, no semantic/AI search).

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

- Build: succeeded, including new `platform/search` module and `ObjectStore`/`RelationshipStore` enumeration support
- `oep_repository_tests`, `oep_engineering_object_tests`, `oep_relationship_tests`, `oep_search_engine_tests` (CTest): 4/4 suites passed
  - Search suite covers: missing-index graceful handling, exact/partial/case-insensitive matches across every searchable field for both objects and relationships, relationship results identifying source/target/type, no-match queries, empty-query rejection, determinism across repeated identical searches, index rebuild dropping a removed object, `clear_index` emptying the index without touching repository contents, and an empty repository producing an empty, valid index
- `oep init my-workshop`: re-verified unaffected, exit code 0

Task 000006 is complete pending formal acceptance.
