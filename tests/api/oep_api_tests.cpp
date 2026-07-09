#include "oep/api/oep_api.h"

#include "oep/repository/metadata.hpp"

#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

int g_failures = 0;

void check(bool condition, const std::string& description) {
    if (!condition) {
        std::cerr << "FAIL: " << description << "\n";
        ++g_failures;
    }
}

std::filesystem::path build_repository(const std::filesystem::path& root) {
    std::filesystem::create_directories(root);

    oep::repository::RepositoryMetadata metadata;
    metadata.repository_id = "1b9e1b02-e845-482a-b299-1e15ffe3932b";
    metadata.repository_name = "my-workshop";
    metadata.repository_version = "1.0.0";
    metadata.foundation_version = "0.1.0";
    metadata.template_version = "1.0";
    metadata.created_utc = "2026-01-01T00:00:00Z";
    oep::repository::save_metadata(root / "repository.json", metadata);

    return root;
}

void test_version_reporting() {
    check(std::string(oep_foundation_version()) == "0.1.0", "oep_foundation_version reports the Foundation version");
    check(oep_api_version() == OEP_API_VERSION, "oep_api_version reports OEP_API_VERSION");
    check(oep_abi_version() == OEP_ABI_VERSION, "oep_abi_version reports OEP_ABI_VERSION");
}

void test_state_to_string_is_deterministic() {
    check(std::string(oep_runtime_state_to_string(OEP_STATE_UNINITIALIZED)) == "Uninitialized",
          "OEP_STATE_UNINITIALIZED stringifies correctly");
    check(std::string(oep_runtime_state_to_string(OEP_STATE_REPOSITORY_OPEN)) == "RepositoryOpen",
          "OEP_STATE_REPOSITORY_OPEN stringifies correctly");
    check(std::string(oep_runtime_state_to_string(OEP_STATE_REPOSITORY_OPEN)) ==
              std::string(oep_runtime_state_to_string(OEP_STATE_REPOSITORY_OPEN)),
          "state_to_string is deterministic across repeated calls");
}

void test_error_code_and_category_strings() {
    check(std::string(oep_error_code_to_string(OEP_ERROR_INVALID_STATE)) == "InvalidState",
          "OEP_ERROR_INVALID_STATE stringifies correctly");
    check(std::string(oep_error_category_to_string(OEP_ERROR_CATEGORY_STATE)) == "State",
          "OEP_ERROR_CATEGORY_STATE stringifies correctly");
}

void test_create_rejects_null_version() {
    OEP_Runtime runtime = oep_runtime_create(nullptr);
    check(runtime == nullptr, "oep_runtime_create returns NULL for a NULL foundation_version");
}

void test_destroy_is_null_safe() {
    oep_runtime_destroy(nullptr); // must not crash
    check(true, "oep_runtime_destroy(NULL) does not crash");
}

void test_null_handle_calls_return_invalid_argument() {
    const oep_result_t init_result = oep_runtime_initialize(nullptr);
    check(!init_result.success && init_result.error_code == OEP_ERROR_INVALID_ARGUMENT,
          "oep_runtime_initialize(NULL) fails with OEP_ERROR_INVALID_ARGUMENT");
    check(init_result.error_category == OEP_ERROR_CATEGORY_VALIDATION,
          "a NULL-handle failure reports OEP_ERROR_CATEGORY_VALIDATION");
    check(std::strlen(init_result.error_message) > 0, "a failed result carries a non-empty error message");

    check(oep_runtime_get_state(nullptr) == OEP_STATE_UNINITIALIZED,
          "oep_runtime_get_state(NULL) returns OEP_STATE_UNINITIALIZED rather than crashing");
}

void test_full_lifecycle(const std::filesystem::path& scratch_dir) {
    const std::filesystem::path root = build_repository(scratch_dir / "lifecycle");

    OEP_Runtime runtime = oep_runtime_create("0.1.0");
    check(runtime != nullptr, "oep_runtime_create succeeds for a valid foundation_version");
    check(oep_runtime_get_state(runtime) == OEP_STATE_UNINITIALIZED, "a fresh handle starts Uninitialized");

    const oep_result_t open_before_init = oep_runtime_open_repository(runtime, root.string().c_str());
    check(!open_before_init.success && open_before_init.error_code == OEP_ERROR_INVALID_STATE,
          "opening a repository before initialize() fails with OEP_ERROR_INVALID_STATE");

    const oep_result_t init_result = oep_runtime_initialize(runtime);
    check(init_result.success, "oep_runtime_initialize succeeds from Uninitialized");
    check(oep_runtime_get_state(runtime) == OEP_STATE_INITIALIZED, "initialize transitions to Initialized");

    const oep_result_t reinit_result = oep_runtime_initialize(runtime);
    check(!reinit_result.success && reinit_result.error_code == OEP_ERROR_INVALID_STATE,
          "re-initializing an already-initialized Runtime fails with OEP_ERROR_INVALID_STATE");

    const oep_result_t open_result = oep_runtime_open_repository(runtime, root.string().c_str());
    check(open_result.success, "oep_runtime_open_repository succeeds for a valid repository");
    check(oep_runtime_get_state(runtime) == OEP_STATE_REPOSITORY_OPEN, "open_repository transitions to RepositoryOpen");

    oep_repository_status_t status;
    const oep_result_t status_result = oep_runtime_get_repository_status(runtime, &status);
    check(status_result.success, "oep_runtime_get_repository_status succeeds while a repository is open");
    check(status.repository_open != 0, "the status reports repository_open");
    check(std::string(status.repository_id) == "1b9e1b02-e845-482a-b299-1e15ffe3932b",
          "the status reports the correct repository_id");
    check(std::string(status.repository_name) == "my-workshop", "the status reports the correct repository_name");
    check(status.loaded_package_count == 0, "the status reports zero loaded packages for an empty repository");

    const oep_result_t close_result = oep_runtime_close_repository(runtime);
    check(close_result.success, "oep_runtime_close_repository succeeds while a repository is open");
    check(oep_runtime_get_state(runtime) == OEP_STATE_REPOSITORY_CLOSED, "close_repository transitions to RepositoryClosed");

    const oep_result_t close_again_result = oep_runtime_close_repository(runtime);
    check(!close_again_result.success && close_again_result.error_code == OEP_ERROR_INVALID_STATE,
          "closing an already-closed repository fails with OEP_ERROR_INVALID_STATE");

    const oep_result_t shutdown_result = oep_runtime_shutdown(runtime);
    check(shutdown_result.success, "oep_runtime_shutdown succeeds");
    check(oep_runtime_get_state(runtime) == OEP_STATE_SHUTDOWN, "shutdown transitions to Shutdown");

    const oep_result_t shutdown_again_result = oep_runtime_shutdown(runtime);
    check(!shutdown_again_result.success && shutdown_again_result.error_code == OEP_ERROR_INVALID_STATE,
          "shutting down an already-shut-down Runtime fails with OEP_ERROR_INVALID_STATE");

    oep_runtime_destroy(runtime);
}

void test_open_repository_reports_not_found(const std::filesystem::path& scratch_dir) {
    OEP_Runtime runtime = oep_runtime_create("0.1.0");
    oep_runtime_initialize(runtime);

    const std::filesystem::path missing = scratch_dir / "does-not-exist";
    const oep_result_t result = oep_runtime_open_repository(runtime, missing.string().c_str());
    check(!result.success, "opening a nonexistent repository fails");
    check(result.error_code == OEP_ERROR_NOT_FOUND, "opening a nonexistent repository reports OEP_ERROR_NOT_FOUND");
    check(result.error_category == OEP_ERROR_CATEGORY_IO, "a NotFound error reports OEP_ERROR_CATEGORY_IO");

    oep_runtime_destroy(runtime);
}

void test_open_repository_rejects_null_path() {
    OEP_Runtime runtime = oep_runtime_create("0.1.0");
    oep_runtime_initialize(runtime);

    const oep_result_t result = oep_runtime_open_repository(runtime, nullptr);
    check(!result.success && result.error_code == OEP_ERROR_INVALID_ARGUMENT,
          "opening with a NULL repository_path fails with OEP_ERROR_INVALID_ARGUMENT");

    oep_runtime_destroy(runtime);
}

void test_repository_status_fails_without_open_repository() {
    OEP_Runtime runtime = oep_runtime_create("0.1.0");
    oep_runtime_initialize(runtime);

    oep_repository_status_t status;
    status.repository_open = 1; // deliberately non-zero, to confirm the API resets it
    const oep_result_t result = oep_runtime_get_repository_status(runtime, &status);
    check(!result.success && result.error_code == OEP_ERROR_INVALID_STATE,
          "getting repository status without an open repository fails with OEP_ERROR_INVALID_STATE");
    check(status.repository_open == 0, "a failed status call zero-initializes the output structure");

    oep_runtime_destroy(runtime);
}

void test_destroy_closes_an_open_repository(const std::filesystem::path& scratch_dir) {
    const std::filesystem::path root = build_repository(scratch_dir / "destroy-open");

    OEP_Runtime runtime = oep_runtime_create("0.1.0");
    oep_runtime_initialize(runtime);
    const oep_result_t open_result = oep_runtime_open_repository(runtime, root.string().c_str());
    check(open_result.success, "opening the fixture repository succeeds for the destroy-while-open test");

    oep_runtime_destroy(runtime); // must not leak or crash even with a repository still open
    check(true, "destroying a handle with an open repository does not crash");
}

} // namespace

int main() {
    const std::filesystem::path scratch_dir = std::filesystem::temp_directory_path() / "oep_api_tests";
    std::filesystem::remove_all(scratch_dir);
    std::filesystem::create_directories(scratch_dir);

    test_version_reporting();
    test_state_to_string_is_deterministic();
    test_error_code_and_category_strings();
    test_create_rejects_null_version();
    test_destroy_is_null_safe();
    test_null_handle_calls_return_invalid_argument();
    test_full_lifecycle(scratch_dir);
    test_open_repository_reports_not_found(scratch_dir);
    test_open_repository_rejects_null_path();
    test_repository_status_fails_without_open_repository();
    test_destroy_closes_an_open_repository(scratch_dir);

    std::filesystem::remove_all(scratch_dir);

    if (g_failures == 0) {
        std::cout << "All Public C API tests passed.\n";
        return 0;
    }
    std::cerr << g_failures << " Public C API test(s) failed.\n";
    return 1;
}
