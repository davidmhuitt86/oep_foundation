#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "oep/packages/package_manager.hpp"
#include "oep/repository/audit_store.hpp"
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
    const oep::validation::RepositoryValidator* validator() const;
    const oep::packages::PackageManager* package_manager() const;

private:
    RuntimeState state_ = RuntimeState::Uninitialized;
    std::string foundation_version_;

    std::filesystem::path repository_root_;
    std::optional<oep::repository::RepositoryMetadata> metadata_;
    std::optional<oep::repository::AuditStore> audit_;
    std::optional<oep::repository::ObjectStore> objects_;
    std::optional<oep::repository::RelationshipStore> relationships_;
    std::optional<oep::search::SearchEngine> search_;
    std::optional<oep::validation::RepositoryValidator> validator_;
    std::optional<oep::packages::PackageManager> packages_;

    void reset_repository_context();
};

} // namespace oep::runtime
