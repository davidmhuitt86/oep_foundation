#include "oep_api_internal.hpp"

#include "oep/runtime/foundation_version.hpp"

namespace {

oep_runtime_state_t to_capi_state(oep::runtime::RuntimeState state) {
    switch (state) {
        case oep::runtime::RuntimeState::Uninitialized: return OEP_STATE_UNINITIALIZED;
        case oep::runtime::RuntimeState::Initialized: return OEP_STATE_INITIALIZED;
        case oep::runtime::RuntimeState::RepositoryOpen: return OEP_STATE_REPOSITORY_OPEN;
        case oep::runtime::RuntimeState::RepositoryClosed: return OEP_STATE_REPOSITORY_CLOSED;
        case oep::runtime::RuntimeState::Shutdown: return OEP_STATE_SHUTDOWN;
    }
    return OEP_STATE_UNINITIALIZED;
}

void zero_status(oep_repository_status_t* out_status) {
    out_status->repository_open = 0;
    out_status->repository_id[0] = '\0';
    out_status->repository_name[0] = '\0';
    out_status->repository_version[0] = '\0';
    out_status->loaded_package_count = 0;
}

} // namespace

namespace oep::api::detail {

void copy_truncated(const std::string& text, char* buffer, std::size_t buffer_size) {
    if (buffer == nullptr || buffer_size == 0) {
        return;
    }
    const std::size_t copy_length = text.size() < buffer_size - 1 ? text.size() : buffer_size - 1;
    std::memcpy(buffer, text.data(), copy_length);
    buffer[copy_length] = '\0';
}

oep_error_category_t category_for_code(oep_error_code_t code) {
    switch (code) {
        case OEP_ERROR_NONE: return OEP_ERROR_CATEGORY_NONE;
        case OEP_ERROR_INVALID_ARGUMENT: return OEP_ERROR_CATEGORY_VALIDATION;
        case OEP_ERROR_INVALID_STATE: return OEP_ERROR_CATEGORY_STATE;
        case OEP_ERROR_NOT_FOUND: return OEP_ERROR_CATEGORY_IO;
        case OEP_ERROR_OPERATION_FAILED: return OEP_ERROR_CATEGORY_IO;
        case OEP_ERROR_INTERNAL: return OEP_ERROR_CATEGORY_INTERNAL;
    }
    return OEP_ERROR_CATEGORY_INTERNAL;
}

oep_result_t make_success_result() {
    oep_result_t result{};
    result.success = 1;
    result.error_code = OEP_ERROR_NONE;
    result.error_category = OEP_ERROR_CATEGORY_NONE;
    result.error_message[0] = '\0';
    return result;
}

oep_result_t make_error_result(oep_error_code_t code, oep_error_category_t category, const std::string& message) {
    oep_result_t result{};
    result.success = 0;
    result.error_code = code;
    result.error_category = category;
    copy_truncated(message, result.error_message, OEP_MAX_ERROR_MESSAGE);
    return result;
}

} // namespace oep::api::detail

using oep::api::detail::category_for_code;
using oep::api::detail::make_error_result;
using oep::api::detail::make_success_result;

extern "C" {

const char* oep_foundation_version(void) {
    return oep::runtime::kFoundationVersion;
}

int oep_api_version(void) {
    return OEP_API_VERSION;
}

int oep_abi_version(void) {
    return OEP_ABI_VERSION;
}

const char* oep_runtime_state_to_string(oep_runtime_state_t state) {
    switch (state) {
        case OEP_STATE_UNINITIALIZED: return "Uninitialized";
        case OEP_STATE_INITIALIZED: return "Initialized";
        case OEP_STATE_REPOSITORY_OPEN: return "RepositoryOpen";
        case OEP_STATE_REPOSITORY_CLOSED: return "RepositoryClosed";
        case OEP_STATE_SHUTDOWN: return "Shutdown";
    }
    return "Uninitialized";
}

const char* oep_error_code_to_string(oep_error_code_t code) {
    switch (code) {
        case OEP_ERROR_NONE: return "None";
        case OEP_ERROR_INVALID_ARGUMENT: return "InvalidArgument";
        case OEP_ERROR_INVALID_STATE: return "InvalidState";
        case OEP_ERROR_NOT_FOUND: return "NotFound";
        case OEP_ERROR_OPERATION_FAILED: return "OperationFailed";
        case OEP_ERROR_INTERNAL: return "Internal";
    }
    return "Internal";
}

const char* oep_error_category_to_string(oep_error_category_t category) {
    switch (category) {
        case OEP_ERROR_CATEGORY_NONE: return "None";
        case OEP_ERROR_CATEGORY_VALIDATION: return "Validation";
        case OEP_ERROR_CATEGORY_STATE: return "State";
        case OEP_ERROR_CATEGORY_IO: return "IO";
        case OEP_ERROR_CATEGORY_INTERNAL: return "Internal";
    }
    return "Internal";
}

OEP_Runtime oep_runtime_create(const char* foundation_version) {
    if (foundation_version == nullptr) {
        return nullptr;
    }
    try {
        return new oep_runtime_impl(std::string(foundation_version));
    } catch (...) {
        return nullptr;
    }
}

void oep_runtime_destroy(OEP_Runtime runtime) {
    if (runtime == nullptr) {
        return;
    }
    try {
        if (runtime->runtime.state() != oep::runtime::RuntimeState::Shutdown) {
            runtime->runtime.shutdown();
        }
    } catch (...) {
        // Destruction must not throw; a failed best-effort shutdown is
        // not a reason to leak the handle.
    }
    delete runtime;
}

oep_result_t oep_runtime_initialize(OEP_Runtime runtime) {
    if (runtime == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "runtime handle is null");
    }
    try {
        const oep::runtime::RuntimeResult result = runtime->runtime.initialize();
        if (!result.success) {
            return make_error_result(OEP_ERROR_INVALID_STATE, category_for_code(OEP_ERROR_INVALID_STATE),
                                      result.error);
        }
        return make_success_result();
    } catch (const std::exception& ex) {
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), ex.what());
    } catch (...) {
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), "unknown internal error");
    }
}

oep_result_t oep_runtime_open_repository(OEP_Runtime runtime, const char* repository_path) {
    if (runtime == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "runtime handle is null");
    }
    if (repository_path == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "repository_path is null");
    }
    try {
        const oep::runtime::RuntimeState state_before = runtime->runtime.state();
        if (state_before != oep::runtime::RuntimeState::Initialized &&
            state_before != oep::runtime::RuntimeState::RepositoryClosed) {
            return make_error_result(OEP_ERROR_INVALID_STATE, category_for_code(OEP_ERROR_INVALID_STATE),
                                      "cannot open a repository from state '" +
                                          oep::runtime::to_string(state_before) + "'");
        }
        const oep::runtime::RuntimeResult result = runtime->runtime.open_repository(repository_path);
        if (!result.success) {
            const oep_error_code_t code =
                result.error.find("could not load repository metadata") != std::string::npos ? OEP_ERROR_NOT_FOUND
                                                                                                : OEP_ERROR_OPERATION_FAILED;
            return make_error_result(code, category_for_code(code), result.error);
        }
        return make_success_result();
    } catch (const std::exception& ex) {
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), ex.what());
    } catch (...) {
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), "unknown internal error");
    }
}

oep_result_t oep_runtime_close_repository(OEP_Runtime runtime) {
    if (runtime == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "runtime handle is null");
    }
    try {
        if (runtime->runtime.state() != oep::runtime::RuntimeState::RepositoryOpen) {
            return make_error_result(OEP_ERROR_INVALID_STATE, category_for_code(OEP_ERROR_INVALID_STATE),
                                      "no repository is currently open");
        }
        const oep::runtime::RuntimeResult result = runtime->runtime.close_repository();
        if (!result.success) {
            return make_error_result(OEP_ERROR_OPERATION_FAILED, category_for_code(OEP_ERROR_OPERATION_FAILED),
                                      result.error);
        }
        return make_success_result();
    } catch (const std::exception& ex) {
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), ex.what());
    } catch (...) {
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), "unknown internal error");
    }
}

oep_result_t oep_runtime_shutdown(OEP_Runtime runtime) {
    if (runtime == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "runtime handle is null");
    }
    try {
        if (runtime->runtime.state() == oep::runtime::RuntimeState::Shutdown) {
            return make_error_result(OEP_ERROR_INVALID_STATE, category_for_code(OEP_ERROR_INVALID_STATE),
                                      "Runtime is already shut down");
        }
        const oep::runtime::RuntimeResult result = runtime->runtime.shutdown();
        if (!result.success) {
            return make_error_result(OEP_ERROR_OPERATION_FAILED, category_for_code(OEP_ERROR_OPERATION_FAILED),
                                      result.error);
        }
        return make_success_result();
    } catch (const std::exception& ex) {
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), ex.what());
    } catch (...) {
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), "unknown internal error");
    }
}

oep_runtime_state_t oep_runtime_get_state(OEP_Runtime runtime) {
    if (runtime == nullptr) {
        return OEP_STATE_UNINITIALIZED;
    }
    try {
        return to_capi_state(runtime->runtime.state());
    } catch (...) {
        return OEP_STATE_UNINITIALIZED;
    }
}

oep_result_t oep_runtime_get_repository_status(OEP_Runtime runtime, oep_repository_status_t* out_status) {
    if (out_status != nullptr) {
        zero_status(out_status);
    }
    if (runtime == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "runtime handle is null");
    }
    if (out_status == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "out_status is null");
    }
    try {
        if (runtime->runtime.state() != oep::runtime::RuntimeState::RepositoryOpen) {
            return make_error_result(OEP_ERROR_INVALID_STATE, category_for_code(OEP_ERROR_INVALID_STATE),
                                      "no repository is currently open");
        }

        const oep::runtime::RuntimeMetadataResult metadata_result = runtime->runtime.current_metadata();
        if (!metadata_result.success) {
            return make_error_result(OEP_ERROR_OPERATION_FAILED, category_for_code(OEP_ERROR_OPERATION_FAILED),
                                      metadata_result.error);
        }

        const oep::runtime::RuntimePackageSetResult package_result = runtime->runtime.current_package_set();
        if (!package_result.success) {
            return make_error_result(OEP_ERROR_OPERATION_FAILED, category_for_code(OEP_ERROR_OPERATION_FAILED),
                                      package_result.error);
        }

        int loaded_count = 0;
        for (const oep::packages::DiscoveredPackage& package : package_result.packages) {
            if (package.state == oep::packages::PackageState::Loaded) {
                ++loaded_count;
            }
        }

        out_status->repository_open = 1;
        oep::api::detail::copy_truncated(metadata_result.metadata.repository_id, out_status->repository_id,
                                          sizeof(out_status->repository_id));
        oep::api::detail::copy_truncated(metadata_result.metadata.repository_name, out_status->repository_name,
                                          sizeof(out_status->repository_name));
        oep::api::detail::copy_truncated(metadata_result.metadata.repository_version,
                                          out_status->repository_version, sizeof(out_status->repository_version));
        out_status->loaded_package_count = loaded_count;

        return make_success_result();
    } catch (const std::exception& ex) {
        zero_status(out_status);
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), ex.what());
    } catch (...) {
        zero_status(out_status);
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), "unknown internal error");
    }
}

} // extern "C"
