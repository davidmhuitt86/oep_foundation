#include "repository_path_option.hpp"

namespace oep::cli::commands {

std::filesystem::path extract_repository_path(std::vector<std::string>& args) {
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--repository" && i + 1 < args.size()) {
            std::filesystem::path path = args[i + 1];
            args.erase(args.begin() + static_cast<std::ptrdiff_t>(i),
                       args.begin() + static_cast<std::ptrdiff_t>(i) + 2);
            return path;
        }
    }
    return std::filesystem::current_path();
}

} // namespace oep::cli::commands
