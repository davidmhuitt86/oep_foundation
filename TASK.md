# TASK
# TASK.md
## Open Engineering Platform (OEP)

Task ID: 000005

Status: Complete

---

# Current Task

Implement the Engineering Object Relationship Model per OEP-SPEC-005-OBJECT_RELATIONSHIP_MODEL.

This delivers a strongly typed `Relationship` connecting two Engineering Objects, validation, serialization, and a Create/Read/Update/Delete/Enumerate store.

---

# Context

TASK-000004 (Engineering Object Model) is complete and accepted.

OEP-SPEC-005-OBJECT_RELATIONSHIP_MODEL.md defines relationship identity, the six initial relationship types, directionality, validation, and store requirements. Graph search, AI reasoning, visualization, and registry synchronization are explicitly out of scope.

---

# Objectives

Complete the following tasks only.

## Objective 1

Add `Relationship` to `platform/repository`: `relationshipId`, `sourceObjectId`, `targetObjectId`, `relationshipType`, `createdUtc`, `author`, `description`, plus the six initial relationship types (References, Contains, DependsOn, ConnectedTo, Documents, Implements).

---

## Objective 2

Implement validation: both objects exist (via the Engineering Object store), relationship type is valid, source and target IDs differ, UUID format is valid for all IDs.

---

## Objective 3

Implement JSON serialization/deserialization.

---

## Objective 4

Implement a Create/Read/Update/Delete/Enumerate-by-object Relationship Store, independent of Studios. All writes are atomic; validation failures never modify repository contents.

---

## Objective 5

Add unit tests validating relationship creation, persistence, loading, validation, enumeration, and deletion.

---

# Explicitly Out of Scope

Do not implement:

- Graph search
- AI reasoning
- Visualization
- Registry synchronization
- Reverse traversal helpers beyond enumerate-by-object
- Runtime, SDK, Studios, Exchange, Networking, Authentication, Plugin system, GUI

These systems belong to future tasks.

---

# Deliverables

- `Relationship` model and validation
- Relationship serialization/deserialization
- CRUD + enumerate Relationship Store
- Unit tests

---

# Acceptance Criteria

This task is complete only when:

- The project builds successfully.
- Relationship model and store exist.
- CRUD operations succeed.
- Validation succeeds (rejects nonexistent objects, invalid type, matching source/target, malformed UUIDs).
- Serialization round-trips successfully.
- Unit tests covering creation, persistence, loading, validation, enumeration, and deletion pass.

---

# Engineering Expectations

Favor readability.

Favor maintainability.

Favor simplicity.

No external JSON library dependency.

Avoid speculative implementation (no graph search, no reverse-traversal API beyond enumerate-by-object).

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

- Build: succeeded
- `oep_repository_tests`, `oep_engineering_object_tests`, `oep_relationship_tests` (CTest): 3/3 suites passed
  - Relationship suite covers: create succeeds when both objects exist, create fails for a nonexistent object, create fails when source equals target, load round-trip, update preserves relationshipId/sourceObjectId/targetObjectId/relationshipType/createdUtc while applying the new description, remove deletes the relationship, list_by_object enumerates both outgoing and incoming edges, and validation rejects matching ids / a malformed UUID
- `oep init my-workshop`: re-verified unaffected, exit code 0

Task 000005 is complete pending formal acceptance.
