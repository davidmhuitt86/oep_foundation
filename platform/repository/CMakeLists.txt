cmake_minimum_required(VERSION 3.25)

add_library(oep_repository STATIC
    src/metadata.cpp
    src/json_value.cpp
)

target_include_directories(oep_repository PUBLIC include)
target_compile_features(oep_repository PUBLIC cxx_std_23)
