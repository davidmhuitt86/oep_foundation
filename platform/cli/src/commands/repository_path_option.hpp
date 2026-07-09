#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace oep::cli::commands {

// Pulls `--repository <path>` out of `args` if present (anywhere),
// returning the repository path (defaulting to the current working
// directory) and leaving the remaining arguments in `args`. Shared by
// every command whose subcommands take a positional ID, so
// `--repository` can't collide with that positional slot.
std::filesystem::path extract_repository_path(std::vector<std::string>& args);

} // namespace oep::cli::commands
