# Exchange

Status: Foundation (Repository Export/Import)

Purpose: Distributes and reconstructs repository content, per OEP-SPEC-017-REPOSITORY_EXPORT and OEP-SPEC-018-REPOSITORY_IMPORT. Exchange does not alter Repository integrity — it only reads (export) or writes through the same validated store operations everything else uses (import).

## Architecture

- `include/oep/exchange/export_manifest.hpp` — public `ExportManifest` and `validate_export_manifest`; `kSupportedExportVersion` is the archive format version this build produces and accepts
- `include/oep/exchange/repository_exporter.hpp` — public `export_repository`: gathers data exclusively through `ObjectStore::list_all` / `RelationshipStore::list_all` / `AuditStore::list_events` / `PackageManager::list_packages`, never reading repository files directly
- `include/oep/exchange/repository_importer.hpp` — public `import_repository`: validates the entire archive before writing anything, then restores content via `ObjectStore`/`RelationshipStore`/`AuditStore`'s `restore` operations (preserving original IDs and timestamps exactly)
- `src/archive_codec.hpp`/`.cpp` — internal (not public) JSON marshaling for the archive document, independent of each store's own on-disk file schema
- `src/export_manifest.cpp`, `src/repository_exporter.cpp`, `src/repository_importer.cpp` — implementation

The export archive is a single deterministic JSON document (not a compressed/binary container), containing an Export Manifest, Repository Metadata, every Engineering Object, every Relationship, the full Audit Log, and — optionally — every currently `Loaded` package's manifest. This avoids an external archive/compression library dependency while still satisfying "a single archive containing" the required sections.

Consumed by `platform/runtime`'s `FoundationRuntime::export_repository`/`import_repository`, which are in turn the only way `platform/cli`'s `export`/`import` commands touch repository content — Exchange, like every other subsystem, is reached exclusively through the Runtime.
