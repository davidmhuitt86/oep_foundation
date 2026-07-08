# Repository

Status: Foundation (Metadata System)

Purpose: Repository-level services. Currently implements the metadata system defined by OEP-SPEC-003-REPOSITORY_METADATA — `RepositoryMetadata`, a loader, a writer, and validation for `repository.json`.

Engineering Object storage, relationships, and the rest of the Repository Engine are not yet implemented.

## Architecture

- `include/oep/repository/metadata.hpp` — public metadata model and `load_metadata` / `save_metadata` / `validate_metadata`
- `src/json_value.hpp` / `src/json_value.cpp` — internal minimal JSON parser/serializer (not part of the public API)
- `src/metadata.cpp` — metadata schema mapping, validation rules, atomic writes

Consumed by the Foundation Generator (`oep init`) to populate `repository.json`.
