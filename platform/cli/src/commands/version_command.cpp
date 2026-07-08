#include "version_command.hpp"

#include <iostream>

namespace oep::cli::commands {

namespace {
constexpr const char* kVersion = "0.1.0";
}

std::string VersionCommand::name() const {
    return "version";
}

std::string VersionCommand::description() const {
    return "Display the OEP CLI version";
}

int VersionCommand::execute(const std::vector<std::string>& /*args*/) const {
    std::cout << "oep version " << kVersion << "\n";
    return 0;
}

} // namespace oep::cli::commands
