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

} // namespace oep::runtime
