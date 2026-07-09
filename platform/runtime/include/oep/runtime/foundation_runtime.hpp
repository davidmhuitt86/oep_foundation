#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "oep/exchange/export_manifest.hpp"
#include "oep/exchange/repository_template.hpp"
#include "oep/exchange/template_manifest.hpp"
#include "oep/packages/package_manager.hpp"
#include "oep/repository/audit_store.hpp"
#include "oep/repository/batch_processor.hpp"
#include "oep/repository/graph_engine.hpp"
#include "oep/repository/metadata.hpp"
#include "oep/repository/object_store.hpp"
#include "oep/repository/relationship_store.hpp"
#include "oep/search/search_engine.hpp"
#include "oep/validation/repository_validator.hpp"

namespace oep::runtime {

// Runtime states, per OEP-SPEC-011-FOUNDATION_RUNTIME. Transitions are
// deterministic: initialize() only succeeds from Uninitialized;
// open_repository() only from Initialized or RepositoryClosed;
// close_repository() only from RepositoryOpen; shutdown() succeeds
// from any state except Shutdown itself, closing an open repository
// first if necessary.
enum class RuntimeState {
    Uninitialized,
    Initialized,
    RepositoryOpen,
    RepositoryClosed,
    Shutdown,
};

std::string to_string(RuntimeState state);

struct RuntimeResult {
    bool success = false;
    std::string error;
};

struct RuntimeMetadataResult {
    bool success = false;
    std::string error;
    oep::repository::RepositoryMetadata metadata;
};

struct RuntimePackageSetResult {
    bool success = false;
    std::string error;
    std::vector<oep::packages::DiscoveredPackage> packages;
};

struct RuntimeRepositoryPathResult {
    bool success = false;
    std::string error;
    std::filesystem::path path;
};

struct RuntimeExportResult {
    bool success = false;
    std::string error;
    oep::exchange::ExportManifest manifest;
};

struct RuntimeImportResult {
    bool success = false;
    std::string error;
    oep::exchange::ExportManifest manifest;
};

struct RuntimeCreateTemplateResult {
    bool success = false;
    std::string error;
    oep::exchange::TemplateManifest manifest;
};

struct RuntimeListTemplatesResult {
    bool success = false;
    std::string error;
    std::vector<oep::exchange::TemplateManifest> templates;
};

struct RuntimeInstantiateTemplateResult {
    bool success = false;
    std::string error;
};

struct RuntimeBatchCreateResult {
    bool success = false;
    std::string error;
    int objects_created = 0;
    int relationships_created = 0;
};

struct RuntimeBatchDeleteResult {
    bool success = false;
    std::string error;
    int objects_deleted = 0;
    int relationships_deleted = 0;
};

struct RuntimeBatchValidateResult {
    // success is true iff the input parsed AND validated with zero
    // findings — a structurally valid-but-non-empty-findings batch is
    // reported via `findings`, not treated as an operational failure.
    bool success = false;
    std::string error;
    std::vector<std::string> findings;
};

// The single entry point through which applications interact with
// Foundation. FoundationRuntime coordinates the Repository, Search,
// Validation, and Package Manager subsystems; it never reimplements
// their logic, and subsystems never locate each other directly —
// callers request services through the Runtime.
//
// Only one repository may be open within a Runtime instance at a time.
// Opening a repository establishes where Engineering Objects,
// Relationships, and Audit Events live within it:
// `<repository>/repository/objects`, `<repository>/repository/relationships`,
// and `<repository>/repository/audit`. Packages are discovered under
// `<repository>/packages`, per OEP-SPEC-002.
class FoundationRuntime {
public:
    // `foundation_version` is the Foundation version this Runtime is
    // running as, used to check package compatibility once a
    // repository is opened.
    explicit FoundationRuntime(std::string foundation_version);

    RuntimeState state() const;

    RuntimeResult initialize();
    RuntimeResult open_repository(std::filesystem::path repository_root);
    RuntimeResult close_repository();
    RuntimeResult shutdown();

    // Repository Context (OEP-SPEC-011 section 5). Available only
    // while a repository is open.
    RuntimeRepositoryPathResult current_repository() const;
    RuntimeMetadataResult current_metadata() const;
    RuntimePackageSetResult current_package_set() const;

    // Service Registry (OEP-SPEC-011 section 6). Return nullptr
    // whenever a repository is not currently open (before
    // initialization, before opening, after closing, or after shutdown).
    const oep::repository::ObjectStore* object_store() const;
    const oep::repository::RelationshipStore* relationship_store() const;
    const oep::repository::AuditStore* audit_store() const;
    const oep::search::SearchEngine* search_engine() const;
    const oep::repository::GraphEngine* graph_engine() const;
    const oep::validation::RepositoryValidator* validator() const;
    const oep::packages::PackageManager* package_manager() const;

    // Exports the currently open repository to a single deterministic
    // archive, per OEP-SPEC-017-REPOSITORY_EXPORT. Requires a
    // repository to be open; delegates entirely to
    // oep::exchange::export_repository.
    RuntimeExportResult export_repository(const std::filesystem::path& output_file, bool include_packages) const;

    // Reconstructs a repository at `destination` from a previously
    // exported archive, per OEP-SPEC-018-REPOSITORY_IMPORT. Does not
    // require (and does not use) a currently open repository —
    // `destination` is a location to create, not one to open — but is
    // exposed on FoundationRuntime so import, like every other
    // repository operation, executes through the Runtime rather than
    // the CLI reconstructing repository contents directly.
    RuntimeImportResult import_repository(const std::filesystem::path& archive_file,
                                           const std::filesystem::path& destination, bool overwrite) const;

    // Captures the currently open repository into a new template
    // stored under `templates_dir`, per OEP-SPEC-019-REPOSITORY_TEMPLATES.
    // Requires a repository to be open.
    RuntimeCreateTemplateResult create_template(const std::filesystem::path& templates_dir,
                                                 const std::string& template_name, const std::string& description,
                                                 const std::string& author, const std::vector<std::string>& tags,
                                                 bool include_packages) const;

    // Enumerates every valid template stored under `templates_dir`.
    // Does not require an open repository.
    RuntimeListTemplatesResult list_templates(const std::filesystem::path& templates_dir) const;

    // Instantiates `template_id` from `templates_dir` into a brand-new
    // repository at `destination`. Does not require (or use) a
    // currently open repository, but remains on FoundationRuntime so
    // instantiation, like every other repository operation, executes
    // through the Runtime.
    RuntimeInstantiateTemplateResult instantiate_template(const std::filesystem::path& templates_dir,
                                                           const std::string& template_id,
                                                           const std::filesystem::path& destination,
                                                           const std::string& new_repository_name) const;

    // Batch operations, per OEP-SPEC-020-BATCH_OPERATIONS. All three
    // require an open repository. Every batch is fully parsed and
    // validated before any execution begins; a validation failure
    // creates/deletes nothing.
    RuntimeBatchCreateResult execute_batch_create(const std::string& batch_json) const;
    RuntimeBatchDeleteResult execute_batch_delete(const std::string& batch_json) const;

    // Parses and validates a batch-create input without executing it.
    // success is false only for a structural/operational problem
    // (malformed JSON, no repository open); a structurally valid batch
    // with validation findings still reports success = true, with the
    // findings listed in `findings`.
    RuntimeBatchValidateResult validate_batch_create(const std::string& batch_json) const;

private:
    RuntimeState state_ = RuntimeState::Uninitialized;
    std::string foundation_version_;

    std::filesystem::path repository_root_;
    std::optional<oep::repository::RepositoryMetadata> metadata_;
    std::optional<oep::repository::AuditStore> audit_;
    std::optional<oep::repository::ObjectStore> objects_;
    std::optional<oep::repository::RelationshipStore> relationships_;
    std::optional<oep::search::SearchEngine> search_;
    std::optional<oep::repository::GraphEngine> graph_;
    std::optional<oep::validation::RepositoryValidator> validator_;
    std::optional<oep::packages::PackageManager> packages_;

    void reset_repository_context();
};

} // namespace oep::runtime
