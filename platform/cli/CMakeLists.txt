cmake_minimum_required(VERSION 3.25)

add_executable(oep
    src/main.cpp
    src/command_registry.cpp
    src/commands/help_command.cpp
    src/commands/init_command.cpp
    src/commands/version_command.cpp
    src/generator/repository_generator.cpp
    src/generator/uuid.cpp
)

target_include_directories(oep PRIVATE include src)
target_compile_features(oep PRIVATE cxx_std_23)
