# CURRENT_SPRINT
# CURRENT_SPRINT.md
## Open Engineering Platform (OEP)

Sprint: 002

Status: Active

---

# Sprint Name

Foundation Generator

---

# Sprint Objective

Implement the Foundation Generator inside the OEP CLI, so that `oep init <repository-name>` produces a Standard Repository conforming to OEP-SPEC-002-FOUNDATION_REPOSITORY.

The goal is not to implement the Repository Engine.

The goal is to generate the correct on-disk structure that future Repository Engine work will operate on.

---

# Primary Deliverable

`oep init`

The Foundation Generator, implemented inside the CLI, becomes the mechanism for creating new OEP repositories.

---

# Sprint Scope

This sprint includes:

- `init` command
- Foundation Repository generator (directories, README.md, .gitignore, repository.json, workspace.json)
- Repository ID generation
- Documentation updates

---

# Out of Scope

The following items are explicitly excluded from this sprint:

- Repository Engine (reading/validating Engineering Objects)
- Runtime
- SDKs
- Exchange
- Studios
- Plugin System
- Authentication
- Networking
- Repository Synchronization
- Marketplace
- Licensing
- Hardware Integration
- Enterprise/Educational/Embedded repository types

If implementation of these items appears necessary, document the dependency and continue working within the current sprint boundaries.

---

# Acceptance Criteria

This sprint is complete when:

- The CLI builds successfully.
- `oep init <repository-name>` creates the repository exactly as specified in OEP-SPEC-002.
- Generation fails safely against an existing non-empty destination.
- The repository remains buildable.
- Documentation is updated.

---

# Definition of Success

At the conclusion of this sprint:

A new developer should be able to clone the repository, build the CLI, execute it, understand the architecture, and be prepared to begin implementing additional commands.

---

# Engineering Principles

During this sprint:

Build only what is necessary.

Avoid feature creep.

Preserve architectural boundaries.

Favor maintainability over speed.

Prefer modularity over convenience.

Do not optimize prematurely.

---

# Deliverables

- OEP CLI executable
- Command registry
- Command interface
- Version command
- Help command
- Initial template framework
- Updated documentation

---

# Risks

Primary risks during this sprint:

- Over-engineering
- Introducing unnecessary dependencies
- Expanding sprint scope
- Breaking architectural boundaries

If any of these occur, stop and reevaluate before continuing.

---

# Next Sprint Preview

Sprint 002

Foundation Generator

Objectives:

- Implement `oep init`
- Generate complete OEP repositories
- Create official repository template
- Validate generated repository
- Prepare for OEP Shell development

This section is informational only and shall not influence implementation during the current sprint.

---

# Completion Review

Before closing the sprint, verify:

✓ Project builds

✓ Documentation updated

✓ Architecture preserved

✓ Repository cleaner than before

✓ Acceptance criteria satisfied

Only then may the sprint be considered complete.

---

# Task 000001 — Verified Complete

`oep --help` and `oep version` were built with MSVC 19.51 (Visual Studio Build Tools 18) via CMake/Ninja and executed successfully (exit code 0 for both). Sprint 001 acceptance criteria are satisfied.

---

# Task 000002 — Verified Complete

`oep init my-workshop` was built and executed with MSVC 19.51 via CMake/Ninja. The generated repository matched OEP-SPEC-002-FOUNDATION_REPOSITORY exactly: `repository/`, `workspace/`, `packages/`, `cache/`, `logs/`, `exports/`, `settings/`, plus `README.md`, `.gitignore`, `repository.json`, and `workspace.json` with a valid UUIDv4 repository ID. Re-running `init` against the same (now non-empty) directory failed safely with exit code 1 and made no changes. Sprint 002 acceptance criteria are satisfied.