# Repository

Status: Foundation (Metadata System, Engineering Object Model, Relationship Model, Graph Traversal)

Purpose: Repository-level services. Implements:

- The metadata system defined by OEP-SPEC-003-REPOSITORY_METADATA — `RepositoryMetadata`, a loader, a writer, and validation for `repository.json`.
- The Engineering Object model defined by OEP-SPEC-004-ENGINEERING_OBJECT_MODEL — `EngineeringObject`, validation, JSON serialization, and a Create/Read/Update/Delete `ObjectStore`.
- The Object Relationship model defined by OEP-SPEC-005-OBJECT_RELATIONSHIP_MODEL — `Relationship`, validation, JSON serialization, and a Create/Read/Update/Delete/Enumerate `RelationshipStore`.
- Repository Graph Traversal defined by OEP-SPEC-007-GRAPH_TRAVERSAL — `GraphEngine`, an in-memory graph over Engineering Objects (nodes) and Relationships (edges) supporting neighbor discovery, BFS/DFS traversal, and path-existence detection.

AI reasoning and the rest of the Repository Engine are not yet implemented. Search is implemented separately in `platform/search`.

## Architecture

- `include/oep/repository/metadata.hpp` — public metadata model and `load_metadata` / `save_metadata` / `validate_metadata`
- `include/oep/repository/engineering_object.hpp` — public `EngineeringObject` model, `ObjectType`, and `validate_object`
- `include/oep/repository/object_store.hpp` — public `ObjectStore` (create/load/update/remove/list_all)
- `include/oep/repository/relationship.hpp` — public `Relationship` model, `RelationshipType`, and `validate_relationship`
- `include/oep/repository/relationship_store.hpp` — public `RelationshipStore` (create/load/update/remove/list_by_object/list_all), validated against an `ObjectStore`
- `include/oep/repository/graph_engine.hpp` — public `GraphEngine` (build_graph/clear_graph/get_neighbors/traverse_breadth_first/traverse_depth_first/path_exists)
- `include/oep/repository/uuid.hpp` — shared UUIDv4 generation/validation, used by metadata, objects, and relationships
- `include/oep/repository/timestamp.hpp` — shared UTC timestamp formatting
- `src/json_value.hpp` / `src/json_value.cpp` — internal minimal JSON parser/serializer (not part of the public API)
- `src/metadata.cpp`, `src/engineering_object.cpp`, `src/object_store.cpp`, `src/relationship.cpp`, `src/relationship_store.cpp`, `src/graph_engine.cpp` — schema mapping, validation rules, atomic writes, graph construction/traversal

Consumed by the Foundation Generator (`oep init`) to populate `repository.json`.
