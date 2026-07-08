#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "oep/cli/command_registry.hpp"
#include "commands/help_command.hpp"
#include "commands/version_command.hpp"

int main(int argc, char** argv) {
    oep::cli::CommandRegistry registry;
    registry.register_command(std::make_unique<oep::cli::commands::VersionCommand>());
    registry.register_command(std::make_unique<oep::cli::commands::HelpCommand>(registry));

    std::vector<std::string> args(argv + 1, argv + argc);

    if (args.empty() || args[0] == "--help" || args[0] == "-h") {
        return registry.find("help")->execute({});
    }

    const std::string& command_name = args[0];
    const oep::cli::Command* command = registry.find(command_name);

    if (command == nullptr) {
        std::cerr << "oep: unknown command '" << command_name << "'\n";
        std::cerr << "Run 'oep --help' for a list of available commands.\n";
        return 1;
    }

    return command->execute(std::vector<std::string>(args.begin() + 1, args.end()));
}
