cmake_minimum_required(VERSION 3.25)
project(OEPFoundation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_subdirectory(platform/repository)
add_subdirectory(platform/search)
add_subdirectory(platform/validation)
add_subdirectory(platform/cli)
add_subdirectory(tests)
