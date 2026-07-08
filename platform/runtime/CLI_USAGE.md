# OEP CLI Usage

## Purpose

`oep` is the primary developer interface for the Open Engineering Platform Foundation. It is a thin presentation layer: every command that touches a repository does so exclusively through `FoundationRuntime`, and never constructs Repository, Search, Validation, or Package services directly. Business logic lives in Foundation; the CLI only presents it.

---

## Build Prerequisites

- A C++23 compiler (verified with MSVC 19.51 / Visual Studio Build Tools 18)
- CMake 3.25+
- A CMake generator with a working build tool (this project has been built and tested with Ninja)

---

## Build Instructions

From the repository root:

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

The `oep` executable is produced at `build/platform/cli/oep` (`oep.exe` on Windows).

To run the test suite:

```bash
cd build
ctest --output-on-failure
```

---

## Running the CLI

```bash
oep --help
oep <command> [arguments]
```

Running `oep` with no arguments, or with `--help`/`-h`, shows the help listing.

---

## Commands

| Command | Syntax | Description |
|---|---|---|
| version | `oep version` | Display the OEP CLI version |
| init | `oep init <repository-name>` | Create a new OEP repository |
| open | `oep open <repository>` | Open a repository through the Foundation Runtime |
| validate | `oep validate [repository]` | Validate repository integrity and report health |
| packages | `oep packages [repository]` | List discovered packages and their state |
| status | `oep status [repository]` | Display Foundation Runtime status and repository information |
| object | `oep object <create\|list\|show\|delete> [arguments] [--repository <path>]` | Create, list, show, and delete Engineering Objects |
| help | `oep help` | Display available commands |

`validate`, `packages`, and `status` default to the current working directory when no repository path is given. `open` and `init` require an explicit argument. `object` subcommands default to the current working directory unless `--repository <path>` is given — a flag rather than a positional argument, since `object show`/`object delete` already use the one positional slot for the object ID.

### Object commands

| Subcommand | Syntax | Description |
|---|---|---|
| create | `oep object create --type <type> --name <name> [--description <text>] [--author <name>] [--tags a,b,c] [--repository <path>]` | Create a new Engineering Object |
| list | `oep object list [--repository <path>]` | List every object (Object ID, Type, Name, Version), sorted by Object ID |
| show | `oep object show <object-id> [--repository <path>]` | Display every field of one object |
| delete | `oep object delete <object-id> [--repository <path>]` | Delete an object (automatically records an `ObjectDeleted` Audit Event) |

`--type` must be one of: `Document`, `Diagram`, `Component`, `Procedure`, `Project`, `Image`. Object IDs are generated automatically by `create` — they are never chosen by the caller.

---

## Example Sessions

### Creating and inspecting a repository

```text
$ oep init my-workshop
created repository 'my-workshop' at /path/to/my-workshop

$ oep status my-workshop
Foundation version: 0.1.0
Runtime state: RepositoryOpen
Repository: my-workshop
Repository ID: 838b6f5b-5f66-4fdf-90f6-fd6df20ac326
Loaded packages: 0

$ oep validate my-workshop
Repository status: Healthy
Errors: 0  Warnings: 0  Information: 0

$ oep packages my-workshop
No packages discovered.
```

### Opening a repository directly

```text
$ oep open my-workshop
Opened repository 'my-workshop' at my-workshop
```

### Creating and managing Engineering Objects

```text
$ oep object create --type Component --name "Ignition Coil" --description "Generates spark" --author "Jane" --tags electrical,ignition
Created object '044ba21d-85b2-4502-a5ea-3787fec41367' (Component) 'Ignition Coil'

$ oep object list
044ba21d-85b2-4502-a5ea-3787fec41367	Component	Ignition Coil	1.0.0

$ oep object show 044ba21d-85b2-4502-a5ea-3787fec41367
Object ID:        044ba21d-85b2-4502-a5ea-3787fec41367
Object Type:      Component
Name:             Ignition Coil
Description:      Generates spark
Author:           Jane
Tags:             electrical, ignition
Version:          1.0.0
Created:          2026-07-08T15:06:52.762Z
Last Modified:    2026-07-08T15:06:52.762Z

$ oep object delete 044ba21d-85b2-4502-a5ea-3787fec41367
Deleted object '044ba21d-85b2-4502-a5ea-3787fec41367'

$ oep object list
No objects found.

$ oep object show 044ba21d-85b2-4502-a5ea-3787fec41367
oep: could not show object: no object with id '044ba21d-85b2-4502-a5ea-3787fec41367'
```

Every `object` subcommand runs against the repository rooted at the current working directory by default; pass `--repository <path>` to target a different one.

### An invalid or missing repository

```text
$ oep validate ./does-not-exist
oep: could not open repository: could not load repository metadata: could not open './does-not-exist/repository.json'
```

`status` handles a missing repository without failing the command itself:

```text
$ oep status ./does-not-exist
Foundation version: 0.1.0
Runtime state: Initialized
Repository: none (could not load repository metadata: could not open './does-not-exist/repository.json')
```

---

## Repository Layout Expected by Foundation

A repository generated by `oep init` (per OEP-SPEC-002-FOUNDATION_REPOSITORY) has this top-level shape:

```text
my-workshop/
├── repository/
├── workspace/
├── packages/
├── cache/
├── logs/
├── exports/
├── settings/
├── README.md
├── .gitignore
├── repository.json
└── workspace.json
```

When `FoundationRuntime` opens a repository, it additionally expects (and will create as needed when writing) Engineering Objects, Relationships, and Audit Events at fixed locations beneath `repository/`:

```text
repository/
├── objects/         Engineering Objects (one JSON file per object)
├── relationships/    Relationships (one JSON file per relationship)
└── audit/            Audit Events (one JSON file per event)
```

Packages are discovered under the repository's top-level `packages/` directory; each package is a subdirectory containing a `package.json` manifest.

---

## Current Limitations

- No persistent session: every CLI invocation is a fresh process. `oep open` opens a repository through `FoundationRuntime`, reports the result, and shuts the Runtime down before exiting — it does not keep a repository open for subsequent commands. `validate`/`packages`/`status` each independently open (and close) the repository they operate on.
- Output is plain, human-readable text only; there is no structured/JSON output mode yet.
- No interactive shell, scripting support, or remote execution.
- `oep init` does not yet populate `repository/objects`, `repository/relationships`, or `repository/audit` — those directories are created on demand by the Runtime-backed commands the first time they're needed.
- Package installation, updates, and dependency resolution are not implemented; `oep packages` only discovers and reports what's already present.
- `oep object` supports create/list/show/delete only — there is no way to edit an existing object's fields from the CLI, import binary assets, or manage Relationships yet.
- `oep object list` always sorts by Object ID; there is no filtering or alternate sort order yet.

---

## Troubleshooting

**"could not open repository: could not load repository metadata"**
The given path isn't an OEP repository, or `repository.json` is missing/corrupt. Confirm the path and that `oep init` completed successfully there.

**A command exits with a non-zero status but no explanation**
Every command prints a descriptive error to stderr before returning a non-zero exit code — check stderr, not just stdout. Commands never surface stack traces or internal exception text; if you see something that looks like one, it indicates a bug worth reporting.

**`oep validate` reports errors on a repository you believe is healthy**
Run `oep validate` again — reports are deterministic for unchanged repository state, so a differing result across runs indicates the repository itself changed between runs, not a flaky validator.
