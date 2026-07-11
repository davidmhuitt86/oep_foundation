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

// Mutation results (Work Package 014). RuntimeResult (success/error only)
// is reused for delete_object/delete_relationship and transaction
// begin/commit/rollback, since none of those need to return a record.
struct RuntimeObjectMutationResult {
    bool success = false;
    std::string error;
    oep::repository::EngineeringObject object;
};

struct RuntimeRelationshipMutationResult {
    bool success = false;
    std::string error;
    oep::repository::Relationship relationship;
};

// Input to batch_create_objects, per OEP-SPEC (Work Package 014,
// TASK-000030). Mirrors the fields create_object accepts individually.
struct ObjectCreateSpec {
    oep::repository::ObjectType object_type = oep::repository::ObjectType::Document;
    std::string name;
    std::string description;
    std::string author;
    std::vector<std::string> tags;
};

// Input to batch_create_relationships. Mirrors the fields
// create_relationship accepts individually.
struct RelationshipCreateSpec {
    std::string source_object_id;
    std::string target_object_id;
    oep::repository::RelationshipType relationship_type = oep::repository::RelationshipType::References;
    std::string author;
    std::string description;
};

struct RuntimeBatchCreateObjectsResult {
    bool success = false;
    std::string error;
    // -1 on full success; otherwise the 0-based index into the input
    // specs of the item that failed. On failure, `created` is always
    // empty — the whole batch is rolled back, per TASK-000030's
    // "Failures shall roll back the complete batch."
    int failed_index = -1;
    std::vector<oep::repository::EngineeringObject> created;
};

struct RuntimeBatchCreateRelationshipsResult {
    bool success = false;
    std::string error;
    int failed_index = -1;
    std::vector<oep::repository::Relationship> created;
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

    // ---------------------------------------------------------------
    // Object and Relationship Mutation (Work Package 014)
    // ---------------------------------------------------------------
    //
    // Every method below requires an open repository and delegates
    // entirely to ObjectStore/RelationshipStore — no validation or
    // persistence logic is duplicated here; Runtime only orchestrates
    // which store method to call and, when a transaction is active,
    // records enough information to undo the call on rollback.
    //
    // If a transaction is active and a mutation fails, the active
    // transaction is automatically rolled back and deactivated before
    // the failure is returned — per TASK-000029, "Failures shall:
    // Abort transaction, Roll back changes." The caller does not need
    // to call rollback_transaction() again in that case (doing so is
    // safe but reports "no transaction is currently active").

    RuntimeObjectMutationResult create_object(oep::repository::ObjectType object_type, const std::string& name,
                                               const std::string& description, const std::string& author,
                                               const std::vector<std::string>& tags);

    // Replaces name/description/author/tags on the object identified
    // by `object_id`; object_id, object_type, and created_utc are
    // preserved, matching ObjectStore::update's own contract exactly
    // (Runtime does not reimplement or extend it).
    RuntimeObjectMutationResult update_object(const std::string& object_id, const std::string& name,
                                               const std::string& description, const std::string& author,
                                               const std::vector<std::string>& tags);

    RuntimeResult delete_object(const std::string& object_id);

    RuntimeRelationshipMutationResult create_relationship(const std::string& source_object_id,
                                                            const std::string& target_object_id,
                                                            oep::repository::RelationshipType relationship_type,
                                                            const std::string& author, const std::string& description);

    // Replaces author/description on the relationship identified by
    // `relationship_id`; relationship_id, source/target object IDs,
    // relationship_type, and created_utc are preserved, matching
    // RelationshipStore::update's own contract exactly.
    RuntimeRelationshipMutationResult update_relationship(const std::string& relationship_id,
                                                            const std::string& author, const std::string& description);

    RuntimeResult delete_relationship(const std::string& relationship_id);

    // ---------------------------------------------------------------
    // Transactions (Work Package 014, TASK-000029)
    // ---------------------------------------------------------------
    //
    // A transaction groups the object/relationship mutations above
    // into one deterministically reversible unit. Only one transaction
    // may be active at a time (no nesting). Each mutation still writes
    // to disk immediately, exactly as it would outside a transaction —
    // ObjectStore/RelationshipStore have no concept of a staged,
    // uncommitted write — but while a transaction is active, Runtime
    // additionally records a compensating action for every successful
    // mutation. commit_transaction() simply discards that log (the
    // mutations are already persisted); rollback_transaction() replays
    // the log in reverse, undoing each mutation via the same stores
    // (remove() undoes a create, update() undoes an update by
    // restoring the prior full record, restore() undoes a delete by
    // restoring the prior full record with its original identity).
    // This makes rollback deterministic without introducing a second,
    // parallel persistence mechanism.
    //
    // Closing the repository (close_repository()/shutdown()) while a
    // transaction is active automatically rolls it back first, so a
    // repository is never left with a transaction stranded open.

    RuntimeResult begin_transaction();
    RuntimeResult commit_transaction();
    RuntimeResult rollback_transaction();
    bool transaction_active() const;

    // ---------------------------------------------------------------
    // Batch Mutation (Work Package 014, TASK-000030)
    // ---------------------------------------------------------------
    //
    // Convenience wrappers over the transaction primitives above:
    // creates every spec in array order (the deterministic execution
    // order TASK-000030 requires) inside a transaction. If no
    // transaction is active when called, one is begun and committed
    // (or rolled back) automatically around the whole batch; if the
    // caller already has a transaction active, the batch participates
    // in it, and a failure rolls back everything in that transaction,
    // not just the batch's own entries.

    RuntimeBatchCreateObjectsResult batch_create_objects(const std::vector<ObjectCreateSpec>& specs);
    RuntimeBatchCreateRelationshipsResult batch_create_relationships(const std::vector<RelationshipCreateSpec>& specs);

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

    // A single compensating action recorded while a transaction is
    // active, per mutation performed. Exactly one of object_snapshot /
    // relationship_snapshot is meaningful for a given `kind`; which one
    // is documented next to each Kind value below.
    struct TransactionLogEntry {
        enum class Kind {
            ObjectCreated,        // undo: remove(object_snapshot.object_id)
            ObjectUpdated,        // undo: update(object_snapshot) — the pre-update record
            ObjectDeleted,        // undo: restore(object_snapshot) — the pre-delete record
            RelationshipCreated,  // undo: remove(relationship_snapshot.relationship_id)
            RelationshipUpdated,  // undo: update(relationship_snapshot) — the pre-update record
            RelationshipDeleted,  // undo: restore(relationship_snapshot) — the pre-delete record
        };
        Kind kind;
        oep::repository::EngineeringObject object_snapshot;
        oep::repository::Relationship relationship_snapshot;
    };

    bool transaction_active_ = false;
    std::vector<TransactionLogEntry> transaction_log_;

    void reset_repository_context();

    // Undoes every recorded entry in transaction_log_, most recent
    // first, then clears the log and deactivates the transaction.
    // Assumes objects_/relationships_ are still valid (must be called
    // before reset_repository_context() clears them, not after).
    void rollback_transaction_internal();
};

} // namespace oep::runtime
