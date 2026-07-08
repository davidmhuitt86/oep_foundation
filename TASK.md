# TASK
# TASK.md
## Open Engineering Platform (OEP)

Task ID: 000012

Status: Complete

---

# Current Task

Implement the CLI Command Framework Expansion per OEP-SPEC-012-CLI_COMMAND_FRAMEWORK.

This expands `oep` from a bootstrap utility into a thin, Runtime-backed developer interface: `oep open`, `oep validate`, `oep packages`, `oep status`, a richer help system, and `platform/runtime/CLI_USAGE.md`.

---

# Context

TASK-000011 (Foundation Runtime) is complete and accepted.

OEP-SPEC-012-CLI_COMMAND_FRAMEWORK.md defines the four new commands, requires every command needing repository access to go through `FoundationRuntime` rather than constructing Repository services directly, requires Name/Syntax/Description in the help system, and requires `platform/runtime/CLI_USAGE.md` as part of the Definition of Done.

To make commands unit-testable without spawning the built executable as a subprocess, `platform/cli` is refactored so command logic lives in a static library (`oep_cli_core`) that both the `oep` executable and the test suite link against; `main.cpp` becomes a thin wrapper.

The Foundation version string ("0.1.0") was previously duplicated between `VersionCommand` and the Foundation Generator; consolidated into one `oep::cli::kFoundationVersion` constant, now also passed to `FoundationRuntime`.

---

# Objectives

Complete the following tasks only.

## Objective 1

Refactor `platform/cli` into `oep_cli_core` (library) + a thin `oep` executable, without changing existing command behavior.

---

## Objective 2

Implement `oep open <repository>`, `oep validate [repository]`, `oep packages [repository]`, `oep status [repository]`, each obtaining services exclusively through `FoundationRuntime` (initialize → open_repository → do the command's work → shutdown). `validate`/`packages`/`status` default to the current working directory when no repository path is given.

---

## Objective 3

Extend the `Command` interface with a `usage()` method (defaulting to `"oep " + name()`, overridden where a command takes arguments) and update `HelpCommand` to print Name, Syntax, and Description for every registered command.

---

## Objective 4

Write `platform/runtime/CLI_USAGE.md`: purpose, build prerequisites/instructions, running the CLI, every implemented command, example sessions with expected output, the repository layout Foundation expects, current limitations, and troubleshooting tips.

---

## Objective 5

Add unit tests (linking `oep_cli_core` directly, no subprocess spawning) verifying: each new command's execution against a real repository, Runtime integration (services obtained only via `FoundationRuntime`), invalid-repository handling, help generation listing all commands, and descriptive (non-crashing, non-stack-trace) error reporting.

---

# Explicitly Out of Scope

Do not implement:

- Interactive shell mode
- Scripting language support
- Remote execution
- Studio user interfaces
- Structured/JSON command output
- Detailed per-command help beyond Name/Syntax/Description
- Runtime, SDK, Exchange, Authentication, Plugin system, GUI beyond what's already implemented

These systems belong to future tasks.

---

# Deliverables

- `oep_cli_core` library + thin `oep` executable
- `open`/`validate`/`packages`/`status` commands
- Extended help system (Name/Syntax/Description)
- `platform/runtime/CLI_USAGE.md`
- Unit tests

---

# Acceptance Criteria

This task is complete only when:

- The project builds successfully.
- `oep open <repository>` opens a repository through Foundation Runtime.
- `oep validate` executes Repository Validation and displays a summary.
- `oep packages` discovers and lists packages with their current state.
- `oep status` displays Runtime state, current repository, repository ID, loaded package count, and Foundation version.
- Help automatically lists every registered command with Name/Syntax/Description.
- No command constructs Repository/Search/Validation/Package services directly — all go through `FoundationRuntime`.
- `platform/runtime/CLI_USAGE.md` exists and accurately documents the current CLI.
- Unit tests covering command execution, Runtime integration, invalid-repository handling, help generation, and error reporting pass.

---

# Engineering Expectations

Favor readability.

Favor maintainability.

Favor simplicity.

Every command is a thin layer over `FoundationRuntime`; no command duplicates Foundation business logic.

Avoid speculative implementation (no shell mode, no scripting, no JSON output, no remote execution).

---

# Completion Checklist

Before marking this task complete, verify:

✓ Build succeeds

✓ Documentation updated (including `platform/runtime/CLI_USAGE.md`)

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

- Build: succeeded, including the `oep_cli_core` library refactor and four new commands
- `oep_repository_tests`, `oep_engineering_object_tests`, `oep_relationship_tests`, `oep_graph_engine_tests`, `oep_audit_store_tests`, `oep_search_engine_tests`, `oep_repository_validator_tests`, `oep_package_manager_tests`, `oep_foundation_runtime_tests`, `oep_cli_commands_tests` (CTest): 10/10 suites passed
- Manual smoke test: `oep init`, `oep status`, `oep open`, `oep validate`, `oep packages`, `oep --help` all run correctly against a real generated repository; `oep validate ./does-not-exist` fails with a descriptive, non-stack-trace error and exit code 1
- `platform/runtime/CLI_USAGE.md`: created, covers purpose, build, running, every command, example sessions with real captured output, repository layout, limitations, and troubleshooting

Task 000012 is complete pending formal acceptance.
