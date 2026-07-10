#pragma once

// Private implementation details for oep_api.cpp. Never installed or
// exposed to API consumers — the public ABI is oep_api.h alone.

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "oep/api/oep_api.h"
#include "oep/runtime/foundation_runtime.hpp"

// The concrete type behind the opaque OEP_Runtime handle. Applications
// only ever see a pointer to this type as `struct oep_runtime_impl*`;
// its layout is never part of the ABI contract.
struct oep_runtime_impl {
    explicit oep_runtime_impl(std::string foundation_version) : runtime(std::move(foundation_version)) {}

    oep::runtime::FoundationRuntime runtime;
};

namespace oep::api::detail {

// Copies `text` into `buffer` (of `buffer_size` bytes), truncating
// rather than overflowing, and always NUL-terminating a non-empty
// buffer.
void copy_truncated(const std::string& text, char* buffer, std::size_t buffer_size);

oep_result_t make_success_result();
oep_result_t make_error_result(oep_error_code_t code, oep_error_category_t category, const std::string& message);

// The category implied by a given code, per the mapping documented in
// oep_api.h next to oep_error_category_t.
oep_error_category_t category_for_code(oep_error_code_t code);

// Converts an internal ObjectType to its C-ABI equivalent. Both
// enumerations are kept in the same declared order deliberately.
oep_object_type_t to_capi_object_type(oep::repository::ObjectType type);

// Fills `out_object` from `object`, truncating any field that exceeds
// its fixed capacity, per oep_object_info_t's documented contract.
void populate_object_info(const oep::repository::EngineeringObject& object, oep_object_info_t* out_object);

} // namespace oep::api::detail
