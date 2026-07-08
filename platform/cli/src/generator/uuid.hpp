#pragma once

#include <string>

namespace oep::cli::generator {

// Generates a random (v4) UUID, used as a repository's globally unique identifier.
std::string generate_uuid_v4();

} // namespace oep::cli::generator
