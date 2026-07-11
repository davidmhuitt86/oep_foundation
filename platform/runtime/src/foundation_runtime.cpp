#include "oep/runtime/foundation_runtime.hpp"

#include "oep/exchange/repository_exporter.hpp"
#include "oep/exchange/repository_importer.hpp"

namespace oep::runtime {

std::string to_string(RuntimeState state) {
    switch (state) {
        case RuntimeState::Uninitialized: return "Uninitialized";
        case RuntimeState::Initialized: return "Initialized";
        case RuntimeState::RepositoryOpen: return "RepositoryOpen";
        case RuntimeState::RepositoryClosed: return "RepositoryClosed";
        case RuntimeState::Shutdown: return "Shutdown";
    }
    return "Uninitialized";
}

FoundationRuntime::FoundationRuntime(std::string foundation_version)
    : foundation_version_(std::move(foundation_version)) {}

RuntimeState FoundationRuntime::state() const {
    return state_;
}

void FoundationRuntime::reset_repository_context() {
    // Never leave a transaction stranded: roll it back while the stores
    // it needs (objects_/relationships_) are still valid, before they
    // are reset below.
    if (transaction_active_) {
        rollback_transaction_internal();
    }

    repository_root_.clear();
    metadata_.reset();
    audit_.reset();
    objects_.reset();
    relationships_.reset();
    search_.reset();
    graph_.reset();
    validator_.reset();
    packages_.reset();
}

void FoundationRuntime::rollback_transaction_internal() {
    for (auto it = transaction_log_.rbegin(); it != transaction_log_.rend(); ++it) {
        switch (it->kind) {
            case TransactionLogEntry::Kind::ObjectCreated:
                objects_->remove(it->object_snapshot.object_id);
                break;
            case TransactionLogEntry::Kind::ObjectUpdated:
                objects_->update(it->object_snapshot);
                break;
            case TransactionLogEntry::Kind::ObjectDeleted:
                objects_->restore(it->object_snapshot);
                break;
            case TransactionLogEntry::Kind::RelationshipCreated:
                relationships_->remove(it->relationship_snapshot.relationship_id);
                break;
            case TransactionLogEntry::Kind::RelationshipUpdated:
                relationships_->update(it->relationship_snapshot);
                break;
            case TransactionLogEntry::Kind::RelationshipDeleted:
                relationships_->restore(it->relationship_snapshot);
                break;
        }
    }
    transaction_log_.clear();
    transaction_active_ = false;
}

RuntimeResult FoundationRuntime::initialize() {
    if (state_ != RuntimeState::Uninitialized) {
        return {false, "cannot initialize Runtime from state '" + to_string(state_) + "'"};
    }
    state_ = RuntimeState::Initialized;
    return {true, ""};
}

RuntimeResult FoundationRuntime::open_repository(std::filesystem::path repository_root) {
    if (state_ != RuntimeState::Initialized && state_ != RuntimeState::RepositoryClosed) {
        if (state_ == RuntimeState::Uninitialized) {
            return {false, "cannot open a repository before the Runtime is initialized"};
        }
        if (state_ == RuntimeState::RepositoryOpen) {
            return {false, "a repository is already open; call close_repository() first"};
        }
        return {false, "cannot open a repository from state '" + to_string(state_) + "'"};
    }

    const oep::repository::LoadMetadataResult loaded_metadata =
        oep::repository::load_metadata(repository_root / "repository.json");
    if (!loaded_metadata.success) {
        return {false, "could not load repository metadata: " + loaded_metadata.error};
    }

    oep::repository::AuditStore audit(repository_root / "repository" / "audit");
    oep::repository::ObjectStore objects(repository_root / "repository" / "objects", audit);
    oep::repository::RelationshipStore relationships(repository_root / "repository" / "relationships", objects,
                                                       audit);

    oep::search::SearchEngine search;
    search.build_index(objects, relationships);

    oep::repository::GraphEngine graph;
    graph.build_graph(objects, relationships);

    oep::validation::RepositoryValidator validator(repository_root / "repository.json", objects, relationships,
                                                    audit);

    oep::packages::PackageManager packages(repository_root / "packages", foundation_version_);

    repository_root_ = std::move(repository_root);
    metadata_ = loaded_metadata.metadata;
    audit_ = std::move(audit);
    objects_ = std::move(objects);
    relationships_ = std::move(relationships);
    search_ = std::move(search);
    graph_ = std::move(graph);
    validator_ = std::move(validator);
    packages_ = std::move(packages);

    state_ = RuntimeState::RepositoryOpen;
    return {true, ""};
}

RuntimeResult FoundationRuntime::close_repository() {
    if (state_ != RuntimeState::RepositoryOpen) {
        return {false, "no repository is currently open (state: '" + to_string(state_) + "')"};
    }

    reset_repository_context();
    state_ = RuntimeState::RepositoryClosed;
    return {true, ""};
}

RuntimeResult FoundationRuntime::shutdown() {
    if (state_ == RuntimeState::Shutdown) {
        return {false, "Runtime has already been shut down"};
    }

    reset_repository_context();
    state_ = RuntimeState::Shutdown;
    return {true, ""};
}

RuntimeRepositoryPathResult FoundationRuntime::current_repository() const {
    if (state_ != RuntimeState::RepositoryOpen) {
        return {false, "no repository is currently open", {}};
    }
    return {true, "", repository_root_};
}

RuntimeMetadataResult FoundationRuntime::current_metadata() const {
    if (state_ != RuntimeState::RepositoryOpen) {
        return {false, "no repository is currently open", {}};
    }
    return {true, "", *metadata_};
}

RuntimePackageSetResult FoundationRuntime::current_package_set() const {
    if (state_ != RuntimeState::RepositoryOpen) {
        return {false, "no repository is currently open", {}};
    }
    const oep::packages::ListPackagesResult listed = packages_->list_packages();
    if (!listed.success) {
        return {false, listed.error, {}};
    }
    return {true, "", listed.packages};
}

const oep::repository::ObjectStore* FoundationRuntime::object_store() const {
    return state_ == RuntimeState::RepositoryOpen ? &(*objects_) : nullptr;
}

const oep::repository::RelationshipStore* FoundationRuntime::relationship_store() const {
    return state_ == RuntimeState::RepositoryOpen ? &(*relationships_) : nullptr;
}

const oep::repository::AuditStore* FoundationRuntime::audit_store() const {
    return state_ == RuntimeState::RepositoryOpen ? &(*audit_) : nullptr;
}

const oep::search::SearchEngine* FoundationRuntime::search_engine() const {
    return state_ == RuntimeState::RepositoryOpen ? &(*search_) : nullptr;
}

const oep::repository::GraphEngine* FoundationRuntime::graph_engine() const {
    return state_ == RuntimeState::RepositoryOpen ? &(*graph_) : nullptr;
}

const oep::validation::RepositoryValidator* FoundationRuntime::validator() const {
    return state_ == RuntimeState::RepositoryOpen ? &(*validator_) : nullptr;
}

const oep::packages::PackageManager* FoundationRuntime::package_manager() const {
    return state_ == RuntimeState::RepositoryOpen ? &(*packages_) : nullptr;
}

RuntimeExportResult FoundationRuntime::export_repository(const std::filesystem::path& output_file,
                                                           bool include_packages) const {
    if (state_ != RuntimeState::RepositoryOpen) {
        return {false, "no repository is currently open", {}};
    }

    const oep::exchange::ExportResult result = oep::exchange::export_repository(
        *metadata_, *objects_, *relationships_, *audit_, include_packages ? &(*packages_) : nullptr, output_file);
    if (!result.success) {
        return {false, result.error, result.manifest};
    }
    return {true, "", result.manifest};
}

RuntimeImportResult FoundationRuntime::import_repository(const std::filesystem::path& archive_file,
                                                          const std::filesystem::path& destination,
                                                          bool overwrite) const {
    const oep::exchange::ImportResult result = oep::exchange::import_repository(archive_file, destination, overwrite);
    if (!result.success) {
        return {false, result.error, result.manifest};
    }
    return {true, "", result.manifest};
}

RuntimeCreateTemplateResult FoundationRuntime::create_template(const std::filesystem::path& templates_dir,
                                                                const std::string& template_name,
                                                                const std::string& description,
                                                                const std::string& author,
                                                                const std::vector<std::string>& tags,
                                                                bool include_packages) const {
    if (state_ != RuntimeState::RepositoryOpen) {
        return {false, "no repository is currently open", {}};
    }

    const oep::exchange::TemplateStore store(templates_dir);
    const oep::exchange::CreateTemplateResult result =
        store.create_template(template_name, description, author, tags, foundation_version_, *objects_,
                               *relationships_, include_packages ? &(*packages_) : nullptr);
    if (!result.success) {
        return {false, result.error, result.manifest};
    }
    return {true, "", result.manifest};
}

RuntimeListTemplatesResult FoundationRuntime::list_templates(const std::filesystem::path& templates_dir) const {
    const oep::exchange::TemplateStore store(templates_dir);
    const oep::exchange::ListTemplatesResult result = store.list_templates();
    if (!result.success) {
        return {false, result.error, {}};
    }
    return {true, "", result.templates};
}

RuntimeInstantiateTemplateResult FoundationRuntime::instantiate_template(const std::filesystem::path& templates_dir,
                                                                          const std::string& template_id,
                                                                          const std::filesystem::path& destination,
                                                                          const std::string& new_repository_name) const {
    const oep::exchange::TemplateStore store(templates_dir);
    const oep::exchange::InstantiateTemplateResult result =
        store.instantiate_template(template_id, destination, new_repository_name);
    if (!result.success) {
        return {false, result.error};
    }
    return {true, ""};
}

RuntimeBatchCreateResult FoundationRuntime::execute_batch_create(const std::string& batch_json) const {
    if (state_ != RuntimeState::RepositoryOpen) {
        return {false, "no repository is currently open", 0, 0};
    }

    const oep::repository::BatchParseResult parsed = oep::repository::parse_batch_create_request(batch_json);
    if (!parsed.success) {
        return {false, parsed.error, 0, 0};
    }

    const oep::repository::BatchCreateResult result =
        oep::repository::execute_batch_create(parsed.request, *objects_, *relationships_);
    if (!result.success) {
        return {false, result.error, result.objects_created, result.relationships_created};
    }
    return {true, "", result.objects_created, result.relationships_created};
}

RuntimeBatchDeleteResult FoundationRuntime::execute_batch_delete(const std::string& batch_json) const {
    if (state_ != RuntimeState::RepositoryOpen) {
        return {false, "no repository is currently open", 0, 0};
    }

    const oep::repository::BatchDeleteParseResult parsed = oep::repository::parse_batch_delete_request(batch_json);
    if (!parsed.success) {
        return {false, parsed.error, 0, 0};
    }

    const oep::repository::BatchDeleteResult result =
        oep::repository::execute_batch_delete(parsed.request, *objects_, *relationships_);
    if (!result.success) {
        return {false, result.error, result.objects_deleted, result.relationships_deleted};
    }
    return {true, "", result.objects_deleted, result.relationships_deleted};
}

RuntimeBatchValidateResult FoundationRuntime::validate_batch_create(const std::string& batch_json) const {
    if (state_ != RuntimeState::RepositoryOpen) {
        return {false, "no repository is currently open", {}};
    }

    const oep::repository::BatchParseResult parsed = oep::repository::parse_batch_create_request(batch_json);
    if (!parsed.success) {
        return {false, parsed.error, {}};
    }

    const std::vector<std::string> findings =
        oep::repository::validate_batch_create_request(parsed.request, *objects_);
    return {true, "", findings};
}

RuntimeObjectMutationResult FoundationRuntime::create_object(oep::repository::ObjectType object_type,
                                                               const std::string& name, const std::string& description,
                                                               const std::string& author,
                                                               const std::vector<std::string>& tags) {
    if (state_ != RuntimeState::RepositoryOpen) {
        return {false, "no repository is currently open", {}};
    }

    oep::repository::EngineeringObject object;
    object.object_type = object_type;
    object.name = name;
    object.description = description;
    object.author = author;
    object.tags = tags;

    const oep::repository::LoadObjectResult result = objects_->create(object);
    if (!result.success) {
        if (transaction_active_) {
            rollback_transaction_internal();
        }
        return {false, result.error, {}};
    }

    if (transaction_active_) {
        TransactionLogEntry entry;
        entry.kind = TransactionLogEntry::Kind::ObjectCreated;
        entry.object_snapshot = result.object;
        transaction_log_.push_back(std::move(entry));
    }

    return {true, "", result.object};
}

RuntimeObjectMutationResult FoundationRuntime::update_object(const std::string& object_id, const std::string& name,
                                                               const std::string& description,
                                                               const std::string& author,
                                                               const std::vector<std::string>& tags) {
    if (state_ != RuntimeState::RepositoryOpen) {
        return {false, "no repository is currently open", {}};
    }

    const oep::repository::LoadObjectResult existing = objects_->load(object_id);
    if (!existing.success) {
        if (transaction_active_) {
            rollback_transaction_internal();
        }
        return {false, existing.error, {}};
    }

    oep::repository::EngineeringObject updated = existing.object;
    updated.name = name;
    updated.description = description;
    updated.author = author;
    updated.tags = tags;

    const oep::repository::ObjectResult result = objects_->update(updated);
    if (!result.success) {
        if (transaction_active_) {
            rollback_transaction_internal();
        }
        return {false, result.error, {}};
    }

    if (transaction_active_) {
        TransactionLogEntry entry;
        entry.kind = TransactionLogEntry::Kind::ObjectUpdated;
        entry.object_snapshot = existing.object; // pre-update record, for undo
        transaction_log_.push_back(std::move(entry));
    }

    const oep::repository::LoadObjectResult reloaded = objects_->load(object_id);
    return {true, "", reloaded.success ? reloaded.object : updated};
}

RuntimeResult FoundationRuntime::delete_object(const std::string& object_id) {
    if (state_ != RuntimeState::RepositoryOpen) {
        return {false, "no repository is currently open"};
    }

    const oep::repository::LoadObjectResult existing = objects_->load(object_id);
    if (!existing.success) {
        if (transaction_active_) {
            rollback_transaction_internal();
        }
        return {false, existing.error};
    }

    const oep::repository::ObjectResult result = objects_->remove(object_id);
    if (!result.success) {
        if (transaction_active_) {
            rollback_transaction_internal();
        }
        return {result.success, result.error};
    }

    if (transaction_active_) {
        TransactionLogEntry entry;
        entry.kind = TransactionLogEntry::Kind::ObjectDeleted;
        entry.object_snapshot = existing.object; // pre-delete record, for undo
        transaction_log_.push_back(std::move(entry));
    }

    return {true, ""};
}

RuntimeRelationshipMutationResult FoundationRuntime::create_relationship(
    const std::string& source_object_id, const std::string& target_object_id,
    oep::repository::RelationshipType relationship_type, const std::string& author, const std::string& description) {
    if (state_ != RuntimeState::RepositoryOpen) {
        return {false, "no repository is currently open", {}};
    }

    oep::repository::Relationship relationship;
    relationship.source_object_id = source_object_id;
    relationship.target_object_id = target_object_id;
    relationship.relationship_type = relationship_type;
    relationship.author = author;
    relationship.description = description;

    const oep::repository::LoadRelationshipResult result = relationships_->create(relationship);
    if (!result.success) {
        if (transaction_active_) {
            rollback_transaction_internal();
        }
        return {false, result.error, {}};
    }

    if (transaction_active_) {
        TransactionLogEntry entry;
        entry.kind = TransactionLogEntry::Kind::RelationshipCreated;
        entry.relationship_snapshot = result.relationship;
        transaction_log_.push_back(std::move(entry));
    }

    return {true, "", result.relationship};
}

RuntimeRelationshipMutationResult FoundationRuntime::update_relationship(const std::string& relationship_id,
                                                                          const std::string& author,
                                                                          const std::string& description) {
    if (state_ != RuntimeState::RepositoryOpen) {
        return {false, "no repository is currently open", {}};
    }

    const oep::repository::LoadRelationshipResult existing = relationships_->load(relationship_id);
    if (!existing.success) {
        if (transaction_active_) {
            rollback_transaction_internal();
        }
        return {false, existing.error, {}};
    }

    oep::repository::Relationship updated = existing.relationship;
    updated.author = author;
    updated.description = description;

    const oep::repository::RelationshipResult result = relationships_->update(updated);
    if (!result.success) {
        if (transaction_active_) {
            rollback_transaction_internal();
        }
        return {false, result.error, {}};
    }

    if (transaction_active_) {
        TransactionLogEntry entry;
        entry.kind = TransactionLogEntry::Kind::RelationshipUpdated;
        entry.relationship_snapshot = existing.relationship; // pre-update record, for undo
        transaction_log_.push_back(std::move(entry));
    }

    const oep::repository::LoadRelationshipResult reloaded = relationships_->load(relationship_id);
    return {true, "", reloaded.success ? reloaded.relationship : updated};
}

RuntimeResult FoundationRuntime::delete_relationship(const std::string& relationship_id) {
    if (state_ != RuntimeState::RepositoryOpen) {
        return {false, "no repository is currently open"};
    }

    const oep::repository::LoadRelationshipResult existing = relationships_->load(relationship_id);
    if (!existing.success) {
        if (transaction_active_) {
            rollback_transaction_internal();
        }
        return {false, existing.error};
    }

    const oep::repository::RelationshipResult result = relationships_->remove(relationship_id);
    if (!result.success) {
        if (transaction_active_) {
            rollback_transaction_internal();
        }
        return {result.success, result.error};
    }

    if (transaction_active_) {
        TransactionLogEntry entry;
        entry.kind = TransactionLogEntry::Kind::RelationshipDeleted;
        entry.relationship_snapshot = existing.relationship; // pre-delete record, for undo
        transaction_log_.push_back(std::move(entry));
    }

    return {true, ""};
}

RuntimeResult FoundationRuntime::begin_transaction() {
    if (state_ != RuntimeState::RepositoryOpen) {
        return {false, "no repository is currently open"};
    }
    if (transaction_active_) {
        return {false, "a transaction is already active; nested transactions are not supported"};
    }
    transaction_active_ = true;
    transaction_log_.clear();
    return {true, ""};
}

RuntimeResult FoundationRuntime::commit_transaction() {
    if (state_ != RuntimeState::RepositoryOpen) {
        return {false, "no repository is currently open"};
    }
    if (!transaction_active_) {
        return {false, "no transaction is currently active"};
    }
    // Every mutation was already persisted as it happened; committing
    // simply discards the undo log rather than replaying anything.
    transaction_log_.clear();
    transaction_active_ = false;
    return {true, ""};
}

RuntimeResult FoundationRuntime::rollback_transaction() {
    if (state_ != RuntimeState::RepositoryOpen) {
        return {false, "no repository is currently open"};
    }
    if (!transaction_active_) {
        return {false, "no transaction is currently active"};
    }
    rollback_transaction_internal();
    return {true, ""};
}

bool FoundationRuntime::transaction_active() const {
    return transaction_active_;
}

RuntimeBatchCreateObjectsResult FoundationRuntime::batch_create_objects(const std::vector<ObjectCreateSpec>& specs) {
    if (state_ != RuntimeState::RepositoryOpen) {
        return {false, "no repository is currently open", -1, {}};
    }

    const bool started_own_transaction = !transaction_active_;
    if (started_own_transaction) {
        const RuntimeResult begin_result = begin_transaction();
        if (!begin_result.success) {
            return {false, begin_result.error, -1, {}};
        }
    }

    std::vector<oep::repository::EngineeringObject> created;
    for (std::size_t i = 0; i < specs.size(); ++i) {
        const RuntimeObjectMutationResult result =
            create_object(specs[i].object_type, specs[i].name, specs[i].description, specs[i].author, specs[i].tags);
        if (!result.success) {
            // create_object() already rolled back and deactivated the
            // transaction, whether we began it just above or the
            // caller already had it active.
            return {false, result.error, static_cast<int>(i), {}};
        }
        created.push_back(result.object);
    }

    if (started_own_transaction) {
        const RuntimeResult commit_result = commit_transaction();
        if (!commit_result.success) {
            return {false, commit_result.error, -1, {}};
        }
    }

    return {true, "", -1, created};
}

RuntimeBatchCreateRelationshipsResult FoundationRuntime::batch_create_relationships(
    const std::vector<RelationshipCreateSpec>& specs) {
    if (state_ != RuntimeState::RepositoryOpen) {
        return {false, "no repository is currently open", -1, {}};
    }

    const bool started_own_transaction = !transaction_active_;
    if (started_own_transaction) {
        const RuntimeResult begin_result = begin_transaction();
        if (!begin_result.success) {
            return {false, begin_result.error, -1, {}};
        }
    }

    std::vector<oep::repository::Relationship> created;
    for (std::size_t i = 0; i < specs.size(); ++i) {
        const RuntimeRelationshipMutationResult result =
            create_relationship(specs[i].source_object_id, specs[i].target_object_id, specs[i].relationship_type,
                                 specs[i].author, specs[i].description);
        if (!result.success) {
            return {false, result.error, static_cast<int>(i), {}};
        }
        created.push_back(result.relationship);
    }

    if (started_own_transaction) {
        const RuntimeResult commit_result = commit_transaction();
        if (!commit_result.success) {
            return {false, commit_result.error, -1, {}};
        }
    }

    return {true, "", -1, created};
}

} // namespace oep::runtime
