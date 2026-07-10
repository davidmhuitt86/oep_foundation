#include "oep/api/oep_api.h"

#include "oep/repository/audit_store.hpp"
#include "oep/repository/metadata.hpp"
#include "oep/repository/object_store.hpp"
#include "oep/repository/relationship_store.hpp"

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

// Builds a repository with the same metadata as build_repository, plus two
// Engineering Objects (one Component with tags, one Document) and one
// Relationship between them, for enumeration/statistics tests.
std::filesystem::path build_populated_repository(const std::filesystem::path& root) {
    build_repository(root);

    oep::repository::AuditStore audit(root / "repository" / "audit");
    oep::repository::ObjectStore objects(root / "repository" / "objects", audit);
    oep::repository::RelationshipStore relationships(root / "repository" / "relationships", objects, audit);

    oep::repository::EngineeringObject coil;
    coil.object_type = oep::repository::ObjectType::Component;
    coil.name = "Ignition Coil";
    coil.description = "Generates spark";
    coil.author = "Jane";
    coil.tags = {"electrical", "ignition"};
    const oep::repository::LoadObjectResult coil_created = objects.create(coil);

    oep::repository::EngineeringObject manual;
    manual.object_type = oep::repository::ObjectType::Document;
    manual.name = "Manual";
    manual.author = "Jane";
    const oep::repository::LoadObjectResult manual_created = objects.create(manual);

    oep::repository::Relationship relationship;
    relationship.source_object_id = manual_created.object.object_id;
    relationship.target_object_id = coil_created.object.object_id;
    relationship.relationship_type = oep::repository::RelationshipType::Documents;
    relationships.create(relationship);

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

void test_object_enumeration(const std::filesystem::path& scratch_dir) {
    const std::filesystem::path root = build_populated_repository(scratch_dir / "enumeration");

    OEP_Runtime runtime = oep_runtime_create("0.1.0");
    oep_runtime_initialize(runtime);
    oep_runtime_open_repository(runtime, root.string().c_str());

    int count = -1;
    const oep_result_t count_result = oep_object_store_get_count(runtime, &count);
    check(count_result.success, "oep_object_store_get_count succeeds while a repository is open");
    check(count == 2, "the object count reflects both created objects");

    oep_object_list_t list;
    const oep_result_t list_result = oep_object_store_list(runtime, &list);
    check(list_result.success, "oep_object_store_list succeeds while a repository is open");
    check(list.count == 2, "the enumerated list contains both objects");
    check(list.items != nullptr, "a non-empty list has a non-NULL items array");

    if (list.count == 2) {
        check(std::string(list.items[0].object_id) < std::string(list.items[1].object_id),
              "the enumerated list is sorted deterministically by object_id");
    }

    bool found_coil = false;
    for (int i = 0; i < list.count; ++i) {
        if (std::string(list.items[i].name) == "Ignition Coil") {
            found_coil = true;
            check(list.items[i].object_type == OEP_OBJECT_TYPE_COMPONENT,
                  "the enumerated Ignition Coil object reports OEP_OBJECT_TYPE_COMPONENT");
            check(std::string(list.items[i].author) == "Jane", "the enumerated object reports the correct author");
            check(list.items[i].tag_count == 2, "the enumerated object reports both tags");
        }
    }
    check(found_coil, "the enumerated list includes the Ignition Coil object");

    // Repeating enumeration produces the same order (determinism across calls).
    oep_object_list_t second_list;
    oep_object_store_list(runtime, &second_list);
    bool same_order = second_list.count == list.count;
    for (int i = 0; same_order && i < list.count; ++i) {
        same_order = std::string(list.items[i].object_id) == std::string(second_list.items[i].object_id);
    }
    check(same_order, "repeated enumeration produces the same deterministic order");

    oep_object_list_release(&list);
    check(list.items == nullptr && list.count == 0, "oep_object_list_release zeroes the released list");
    oep_object_list_release(&second_list);

    // Releasing an already-released (zeroed) list is a safe no-op.
    oep_object_list_release(&list);
    check(true, "releasing an already-released list does not crash");

    oep_object_list_release(nullptr);
    check(true, "oep_object_list_release(NULL) does not crash");

    oep_runtime_destroy(runtime);
}

void test_object_lookup_by_id(const std::filesystem::path& scratch_dir) {
    const std::filesystem::path root = build_populated_repository(scratch_dir / "lookup");

    OEP_Runtime runtime = oep_runtime_create("0.1.0");
    oep_runtime_initialize(runtime);
    oep_runtime_open_repository(runtime, root.string().c_str());

    oep_object_list_t list;
    oep_object_store_list(runtime, &list);
    check(list.count == 2, "the fixture repository has exactly two objects for the lookup test");

    if (list.count == 2) {
        oep_object_info_t looked_up;
        const oep_result_t lookup_result = oep_object_store_get_by_id(runtime, list.items[0].object_id, &looked_up);
        check(lookup_result.success, "looking up an existing object by ID succeeds");
        check(std::string(looked_up.object_id) == std::string(list.items[0].object_id),
              "the looked-up object has the requested object_id");
        check(std::string(looked_up.name) == std::string(list.items[0].name),
              "the looked-up object has the requested name");
    }
    oep_object_list_release(&list);

    oep_object_info_t missing;
    const oep_result_t missing_result =
        oep_object_store_get_by_id(runtime, "00000000-0000-4000-8000-000000000000", &missing);
    check(!missing_result.success && missing_result.error_code == OEP_ERROR_NOT_FOUND,
          "looking up a nonexistent object ID fails with OEP_ERROR_NOT_FOUND");
    check(std::string(missing.object_id).empty(), "a failed lookup zero-initializes the output structure");

    const oep_result_t null_id_result = oep_object_store_get_by_id(runtime, nullptr, &missing);
    check(!null_id_result.success && null_id_result.error_code == OEP_ERROR_INVALID_ARGUMENT,
          "looking up with a NULL object_id fails with OEP_ERROR_INVALID_ARGUMENT");

    oep_runtime_destroy(runtime);
}

void test_object_enumeration_requires_open_repository() {
    OEP_Runtime runtime = oep_runtime_create("0.1.0");
    oep_runtime_initialize(runtime);

    int count = -1;
    const oep_result_t count_result = oep_object_store_get_count(runtime, &count);
    check(!count_result.success && count_result.error_code == OEP_ERROR_INVALID_STATE,
          "getting the object count without an open repository fails with OEP_ERROR_INVALID_STATE");
    check(count == 0, "a failed count call resets out_count to zero");

    oep_object_list_t list;
    const oep_result_t list_result = oep_object_store_list(runtime, &list);
    check(!list_result.success && list_result.error_code == OEP_ERROR_INVALID_STATE,
          "listing objects without an open repository fails with OEP_ERROR_INVALID_STATE");
    check(list.items == nullptr && list.count == 0, "a failed list call is zero-initialized");

    oep_runtime_destroy(runtime);
}

void test_repository_statistics(const std::filesystem::path& scratch_dir) {
    const std::filesystem::path root = build_populated_repository(scratch_dir / "statistics");

    OEP_Runtime runtime = oep_runtime_create("0.1.0");
    oep_runtime_initialize(runtime);
    oep_runtime_open_repository(runtime, root.string().c_str());

    oep_repository_statistics_t statistics;
    const oep_result_t statistics_result = oep_runtime_get_repository_statistics(runtime, &statistics);
    check(statistics_result.success, "oep_runtime_get_repository_statistics succeeds while a repository is open");
    check(std::string(statistics.repository_id) == "1b9e1b02-e845-482a-b299-1e15ffe3932b",
          "statistics report the correct repository_id");
    check(std::string(statistics.repository_name) == "my-workshop", "statistics report the correct repository_name");
    check(statistics.total_object_count == 2, "statistics report the correct total object count");
    check(statistics.object_count_by_type[OEP_OBJECT_TYPE_COMPONENT] == 1,
          "statistics report exactly one Component object");
    check(statistics.object_count_by_type[OEP_OBJECT_TYPE_DOCUMENT] == 1,
          "statistics report exactly one Document object");
    check(statistics.object_count_by_type[OEP_OBJECT_TYPE_DIAGRAM] == 0,
          "statistics report zero for an object type not present in the repository");
    check(statistics.relationship_count == 1, "statistics report the correct relationship count");
    check(statistics.package_count == 0, "statistics report zero packages for a package-free repository");

    oep_runtime_destroy(runtime);
}

void test_repository_statistics_requires_open_repository() {
    OEP_Runtime runtime = oep_runtime_create("0.1.0");
    oep_runtime_initialize(runtime);

    oep_repository_statistics_t statistics;
    statistics.total_object_count = 99; // deliberately nonzero, to confirm the API resets it
    const oep_result_t result = oep_runtime_get_repository_statistics(runtime, &statistics);
    check(!result.success && result.error_code == OEP_ERROR_INVALID_STATE,
          "getting repository statistics without an open repository fails with OEP_ERROR_INVALID_STATE");
    check(statistics.total_object_count == 0, "a failed statistics call zero-initializes the output structure");

    oep_runtime_destroy(runtime);
}

void test_object_type_to_string() {
    check(std::string(oep_object_type_to_string(OEP_OBJECT_TYPE_COMPONENT)) == "Component",
          "OEP_OBJECT_TYPE_COMPONENT stringifies correctly");
    check(std::string(oep_object_type_to_string(OEP_OBJECT_TYPE_DOCUMENT)) == "Document",
          "OEP_OBJECT_TYPE_DOCUMENT stringifies correctly");
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
    test_object_enumeration(scratch_dir);
    test_object_lookup_by_id(scratch_dir);
    test_object_enumeration_requires_open_repository();
    test_repository_statistics(scratch_dir);
    test_repository_statistics_requires_open_repository();
    test_object_type_to_string();
    test_destroy_closes_an_open_repository(scratch_dir);

    std::filesystem::remove_all(scratch_dir);

    if (g_failures == 0) {
        std::cout << "All Public C API tests passed.\n";
        return 0;
    }
    std::cerr << g_failures << " Public C API test(s) failed.\n";
    return 1;
}
