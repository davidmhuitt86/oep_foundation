# CLI

Status: Foundation (v0.1.0)

Purpose: The `oep` executable — the primary developer tool for creating and managing OEP repositories.

## Architecture

- `include/oep/cli/` — public command interfaces (`Command`, `CommandRegistry`)
- `src/commands/` — individual command implementations
- `src/generator/` — Foundation Generator (repository scaffolding used by `init`)
- `src/main.cpp` — argument parsing and command dispatch

## Commands

- `oep --help` / `oep -h` — display available commands
- `oep version` — display the CLI version
- `oep init <repository-name>` — generate a Standard Repository per OEP-SPEC-002-FOUNDATION_REPOSITORY

New commands are added by implementing `oep::cli::Command` and registering an instance in `main.cpp`. No other part of the CLI needs to change.
