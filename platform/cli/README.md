# CLI

Status: Foundation (v0.1.0)

Purpose: The `oep` executable — the primary developer interface for Foundation, per OEP-SPEC-012-CLI_COMMAND_FRAMEWORK. Every command that touches a repository goes through `FoundationRuntime`; the CLI never constructs Repository/Search/Validation/Package services directly.

## Architecture

- `include/oep/cli/` — public command interfaces (`Command`, `CommandRegistry`)
- `src/commands/` — individual command implementations
- `src/generator/` — Foundation Generator (repository scaffolding used by `init`)
- `src/foundation_version.hpp` — the single `kFoundationVersion` constant shared by `version`, the generator, and every Runtime-backed command
- `src/main.cpp` — thin entry point: registers commands and dispatches

Command logic is built as a static library, `oep_cli_core` (see `CMakeLists.txt`), which the `oep` executable links against. This lets `tests/cli` unit-test commands directly, without spawning the built executable as a subprocess.

## Commands

- `oep --help` / `oep -h` — display available commands (Name/Syntax/Description for each)
- `oep version` — display the CLI version
- `oep init <repository-name>` — generate a Standard Repository per OEP-SPEC-002-FOUNDATION_REPOSITORY
- `oep open <repository>` — open a repository through the Foundation Runtime
- `oep validate [repository]` — run Repository Validation and report health
- `oep packages [repository]` — list discovered packages and their state
- `oep status [repository]` — show Runtime state, current repository, repository ID, and loaded package count
- `oep object create|list|show|delete` — create, list, show, and delete Engineering Objects (`--repository <path>` optional, defaults to the current working directory)

See [platform/runtime/CLI_USAGE.md](../runtime/CLI_USAGE.md) for build instructions, example sessions, and troubleshooting.

New commands are added by implementing `oep::cli::Command` (overriding `usage()` if the command takes arguments) and registering an instance in `main.cpp`. No other part of the CLI needs to change.
