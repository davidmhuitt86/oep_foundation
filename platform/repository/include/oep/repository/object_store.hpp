#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "oep/repository/audit_store.hpp"
#include "oep/repository/engineering_object.hpp"

namespace oep::repository {

struct ObjectResult {
    bool success = false;
    std::string error;
};

struct LoadObjectResult {
    bool success = false;
    std::string error;
    EngineeringObject object;
};

struct ListObjectsResult {
    bool success = false;
    std::string error;
    std::vector<EngineeringObject> objects;
};

// Stores Engineering Objects as individual JSON files within a directory,
// implementing the Create/Read/Update/Delete lifecycle from
// OEP-SPEC-004-ENGINEERING_OBJECT_MODEL. All writes are atomic; a
// validation failure never modifies repository contents.
//
// Every successful create/update/remove automatically records a
// corresponding AuditEvent via `audit`, per
// OEP-SPEC-008-REPOSITORY_AUDIT_LOG — applications never create audit
// events directly. Audit recording never causes an otherwise-successful
// operation to fail.
class ObjectStore {
public:
    ObjectStore(std::filesystem::path root, AuditStore audit);

    // Assigns a new object_id (if not already set) and timestamps, then saves.
    LoadObjectResult create(EngineeringObject object) const;

    LoadObjectResult load(const std::string& object_id) const;

    // Updates an existing object. object_id, object_type, and created_utc
    // are preserved from the stored object; last_modified_utc is refreshed.
    ObjectResult update(EngineeringObject object) const;

    ObjectResult remove(const std::string& object_id) const;

    // Enumerates every valid object stored in this store. Files that are
    // not valid Engineering Objects (e.g. corrupt or mid-write) are skipped.
    ListObjectsResult list_all() const;

private:
    std::filesystem::path root_;
    AuditStore audit_;
    std::filesystem::path path_for(const std::string& object_id) const;
    void record_audit(AuditEventType event_type, const std::string& object_id) const;
};

} // namespace oep::repository
