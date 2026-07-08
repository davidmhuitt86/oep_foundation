#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "oep/repository/audit_store.hpp"
#include "oep/repository/object_store.hpp"
#include "oep/repository/relationship.hpp"

namespace oep::repository {

struct RelationshipResult {
    bool success = false;
    std::string error;
};

struct LoadRelationshipResult {
    bool success = false;
    std::string error;
    Relationship relationship;
};

struct ListRelationshipsResult {
    bool success = false;
    std::string error;
    std::vector<Relationship> relationships;
};

// Stores relationships as individual JSON files within a directory,
// implementing the Create/Read/Update/Delete/Enumerate lifecycle from
// OEP-SPEC-005-OBJECT_RELATIONSHIP_MODEL. Uses `objects` to verify that
// a relationship's source and target Engineering Objects exist. All
// writes are atomic; a validation failure never modifies repository
// contents.
//
// Every successful create/update/remove automatically records a
// corresponding AuditEvent via `audit`, per
// OEP-SPEC-008-REPOSITORY_AUDIT_LOG.
class RelationshipStore {
public:
    RelationshipStore(std::filesystem::path root, ObjectStore objects, AuditStore audit);

    // Assigns a new relationship_id (if not already set) and created_utc,
    // verifies both objects exist, then saves.
    LoadRelationshipResult create(Relationship relationship) const;

    LoadRelationshipResult load(const std::string& relationship_id) const;

    // Updates an existing relationship. relationship_id, source_object_id,
    // target_object_id, relationship_type, and created_utc are preserved
    // from the stored relationship; only author and description may change.
    RelationshipResult update(Relationship relationship) const;

    RelationshipResult remove(const std::string& relationship_id) const;

    // Enumerates every relationship where `object_id` is the source or the target.
    ListRelationshipsResult list_by_object(const std::string& object_id) const;

    // Enumerates every valid relationship stored in this store. Files that
    // are not valid relationships (e.g. corrupt or mid-write) are skipped.
    ListRelationshipsResult list_all() const;

private:
    std::filesystem::path root_;
    ObjectStore objects_;
    AuditStore audit_;
    std::filesystem::path path_for(const std::string& relationship_id) const;
    void record_audit(AuditEventType event_type, const std::string& relationship_id) const;
};

} // namespace oep::repository
