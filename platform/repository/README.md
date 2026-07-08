# Repository

Status: Foundation (Metadata System, Engineering Object Model, Relationship Model)

Purpose: Repository-level services. Implements:

- The metadata system defined by OEP-SPEC-003-REPOSITORY_METADATA — `RepositoryMetadata`, a loader, a writer, and validation for `repository.json`.
- The Engineering Object model defined by OEP-SPEC-004-ENGINEERING_OBJECT_MODEL — `EngineeringObject`, validation, JSON serialization, and a Create/Read/Update/Delete `ObjectStore`.
- The Object Relationship model defined by OEP-SPEC-005-OBJECT_RELATIONSHIP_MODEL — `Relationship`, validation, JSON serialization, and a Create/Read/Update/Delete/Enumerate `RelationshipStore`.

Graph search, AI reasoning, and the rest of the Repository Engine are not yet implemented.

## Architecture

- `include/oep/repository/metadata.hpp` — public metadata model and `load_metadata` / `save_metadata` / `validate_metadata`
- `include/oep/repository/engineering_object.hpp` — public `EngineeringObject` model, `ObjectType`, and `validate_object`
- `include/oep/repository/object_store.hpp` — public `ObjectStore` (create/load/update/remove)
- `include/oep/repository/relationship.hpp` — public `Relationship` model, `RelationshipType`, and `validate_relationship`
- `include/oep/repository/relationship_store.hpp` — public `RelationshipStore` (create/load/update/remove/list_by_object), validated against an `ObjectStore`
- `include/oep/repository/uuid.hpp` — shared UUIDv4 generation/validation, used by metadata, objects, and relationships
- `include/oep/repository/timestamp.hpp` — shared UTC timestamp formatting
- `src/json_value.hpp` / `src/json_value.cpp` — internal minimal JSON parser/serializer (not part of the public API)
- `src/metadata.cpp`, `src/engineering_object.cpp`, `src/object_store.cpp`, `src/relationship.cpp`, `src/relationship_store.cpp` — schema mapping, validation rules, atomic writes

Consumed by the Foundation Generator (`oep init`) to populate `repository.json`.
