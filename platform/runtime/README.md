# Runtime

Status: Foundation (Foundation Runtime)

Purpose: The single entry point through which applications interact with Foundation, per OEP-SPEC-011-FOUNDATION_RUNTIME. Coordinates lifecycle and service access for Repository, Search, Graph, Validation, Package Manager, and Exchange without reimplementing any of them.

## Architecture

- `include/oep/runtime/foundation_runtime.hpp` — public `FoundationRuntime` (`initialize` / `open_repository` / `close_repository` / `shutdown`, Repository Context accessors, Service Registry accessors, `export_repository` / `import_repository`) and `RuntimeState`
- `src/foundation_runtime.cpp` — deterministic state transitions and subsystem construction/registration

`open_repository` establishes where an opened repository's Engineering Objects, Relationships, and Audit Events live on disk: `repository/objects/`, `repository/relationships/`, and `repository/audit/` beneath the repository root, with packages discovered under the repository's existing top-level `packages/` directory. This convention is not yet wired into `oep init`/the Foundation Generator.

Only one repository may be open at a time. Every service accessor (`object_store`, `relationship_store`, `audit_store`, `search_engine`, `graph_engine`, `validator`, `package_manager`) returns `nullptr` unless a repository is currently open — before initialization, before opening, after closing, and after shutdown all behave identically in this respect. `export_repository` requires an open repository (it exports *the* open one); `import_repository` does not — it targets a destination to create, not one to open, and delegates entirely to `oep::exchange::import_repository`.
