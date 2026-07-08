#pragma once

#include <filesystem>
#include <string>

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

// Stores Engineering Objects as individual JSON files within a directory,
// implementing the Create/Read/Update/Delete lifecycle from
// OEP-SPEC-004-ENGINEERING_OBJECT_MODEL. All writes are atomic; a
// validation failure never modifies repository contents.
class ObjectStore {
public:
    explicit ObjectStore(std::filesystem::path root);

    // Assigns a new object_id (if not already set) and timestamps, then saves.
    LoadObjectResult create(EngineeringObject object) const;

    LoadObjectResult load(const std::string& object_id) const;

    // Updates an existing object. object_id, object_type, and created_utc
    // are preserved from the stored object; last_modified_utc is refreshed.
    ObjectResult update(EngineeringObject object) const;

    ObjectResult remove(const std::string& object_id) const;

private:
    std::filesystem::path root_;
    std::filesystem::path path_for(const std::string& object_id) const;
};

} // namespace oep::repository
