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

void zero_object_info(oep_object_info_t* out_object) {
    out_object->object_id[0] = '\0';
    out_object->object_type = OEP_OBJECT_TYPE_DOCUMENT;
    out_object->name[0] = '\0';
    out_object->author[0] = '\0';
    out_object->version[0] = '\0';
    out_object->description[0] = '\0';
    out_object->tag_count = 0;
    for (int i = 0; i < OEP_MAX_OBJECT_TAGS; ++i) {
        out_object->tags[i][0] = '\0';
    }
}

void zero_object_list(oep_object_list_t* out_list) {
    out_list->items = nullptr;
    out_list->count = 0;
}

void zero_statistics(oep_repository_statistics_t* out_statistics) {
    out_statistics->repository_id[0] = '\0';
    out_statistics->repository_name[0] = '\0';
    out_statistics->repository_version[0] = '\0';
    out_statistics->total_object_count = 0;
    for (int i = 0; i < OEP_OBJECT_TYPE_COUNT; ++i) {
        out_statistics->object_count_by_type[i] = 0;
    }
    out_statistics->relationship_count = 0;
    out_statistics->package_count = 0;
}

void zero_relationship_info(oep_relationship_info_t* out_relationship) {
    out_relationship->relationship_id[0] = '\0';
    out_relationship->source_object_id[0] = '\0';
    out_relationship->target_object_id[0] = '\0';
    out_relationship->relationship_type = OEP_RELATIONSHIP_TYPE_REFERENCES;
    out_relationship->author[0] = '\0';
    out_relationship->description[0] = '\0';
    out_relationship->created_utc[0] = '\0';
}

void zero_relationship_list(oep_relationship_list_t* out_list) {
    out_list->items = nullptr;
    out_list->count = 0;
}

void zero_object_search_result_list(oep_object_search_result_list_t* out_list) {
    out_list->items = nullptr;
    out_list->count = 0;
}

void zero_relationship_search_result_list(oep_relationship_search_result_list_t* out_list) {
    out_list->items = nullptr;
    out_list->count = 0;
}

void zero_repository_search_result(oep_repository_search_result_t* out_result) {
    zero_object_search_result_list(&out_result->objects);
    zero_relationship_search_result_list(&out_result->relationships);
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

oep_object_type_t to_capi_object_type(oep::repository::ObjectType type) {
    switch (type) {
        case oep::repository::ObjectType::Document: return OEP_OBJECT_TYPE_DOCUMENT;
        case oep::repository::ObjectType::Diagram: return OEP_OBJECT_TYPE_DIAGRAM;
        case oep::repository::ObjectType::Component: return OEP_OBJECT_TYPE_COMPONENT;
        case oep::repository::ObjectType::Procedure: return OEP_OBJECT_TYPE_PROCEDURE;
        case oep::repository::ObjectType::Project: return OEP_OBJECT_TYPE_PROJECT;
        case oep::repository::ObjectType::Image: return OEP_OBJECT_TYPE_IMAGE;
    }
    return OEP_OBJECT_TYPE_DOCUMENT;
}

void populate_object_info(const oep::repository::EngineeringObject& object, oep_object_info_t* out_object) {
    copy_truncated(object.object_id, out_object->object_id, sizeof(out_object->object_id));
    out_object->object_type = to_capi_object_type(object.object_type);
    copy_truncated(object.name, out_object->name, sizeof(out_object->name));
    copy_truncated(object.author, out_object->author, sizeof(out_object->author));
    copy_truncated(object.version, out_object->version, sizeof(out_object->version));
    copy_truncated(object.description, out_object->description, sizeof(out_object->description));

    const int tag_count =
        static_cast<int>(object.tags.size()) < OEP_MAX_OBJECT_TAGS ? static_cast<int>(object.tags.size())
                                                                    : OEP_MAX_OBJECT_TAGS;
    out_object->tag_count = tag_count;
    for (int i = 0; i < tag_count; ++i) {
        copy_truncated(object.tags[static_cast<std::size_t>(i)], out_object->tags[i], OEP_MAX_TAG_LENGTH);
    }
    for (int i = tag_count; i < OEP_MAX_OBJECT_TAGS; ++i) {
        out_object->tags[i][0] = '\0';
    }
}

oep_relationship_type_t to_capi_relationship_type(oep::repository::RelationshipType type) {
    switch (type) {
        case oep::repository::RelationshipType::References: return OEP_RELATIONSHIP_TYPE_REFERENCES;
        case oep::repository::RelationshipType::Contains: return OEP_RELATIONSHIP_TYPE_CONTAINS;
        case oep::repository::RelationshipType::DependsOn: return OEP_RELATIONSHIP_TYPE_DEPENDS_ON;
        case oep::repository::RelationshipType::ConnectedTo: return OEP_RELATIONSHIP_TYPE_CONNECTED_TO;
        case oep::repository::RelationshipType::Documents: return OEP_RELATIONSHIP_TYPE_DOCUMENTS;
        case oep::repository::RelationshipType::Implements: return OEP_RELATIONSHIP_TYPE_IMPLEMENTS;
    }
    return OEP_RELATIONSHIP_TYPE_REFERENCES;
}

void populate_relationship_info(const oep::repository::Relationship& relationship,
                                 oep_relationship_info_t* out_relationship) {
    copy_truncated(relationship.relationship_id, out_relationship->relationship_id,
                   sizeof(out_relationship->relationship_id));
    copy_truncated(relationship.source_object_id, out_relationship->source_object_id,
                   sizeof(out_relationship->source_object_id));
    copy_truncated(relationship.target_object_id, out_relationship->target_object_id,
                   sizeof(out_relationship->target_object_id));
    out_relationship->relationship_type = to_capi_relationship_type(relationship.relationship_type);
    copy_truncated(relationship.author, out_relationship->author, sizeof(out_relationship->author));
    copy_truncated(relationship.description, out_relationship->description, sizeof(out_relationship->description));
    copy_truncated(relationship.created_utc, out_relationship->created_utc, sizeof(out_relationship->created_utc));
}

oep_match_location_t to_capi_match_location(oep::search::MatchLocation location) {
    switch (location) {
        case oep::search::MatchLocation::Name: return OEP_MATCH_LOCATION_NAME;
        case oep::search::MatchLocation::Description: return OEP_MATCH_LOCATION_DESCRIPTION;
        case oep::search::MatchLocation::Author: return OEP_MATCH_LOCATION_AUTHOR;
        case oep::search::MatchLocation::Tags: return OEP_MATCH_LOCATION_TAGS;
        case oep::search::MatchLocation::ObjectType: return OEP_MATCH_LOCATION_OBJECT_TYPE;
        case oep::search::MatchLocation::RelationshipType: return OEP_MATCH_LOCATION_RELATIONSHIP_TYPE;
    }
    return OEP_MATCH_LOCATION_NAME;
}

void populate_object_search_result(const oep::search::ObjectSearchResult& result,
                                    oep_object_search_result_t* out_result) {
    copy_truncated(result.object_id, out_result->object_id, sizeof(out_result->object_id));
    out_result->object_type = to_capi_object_type(result.object_type);
    copy_truncated(result.display_name, out_result->display_name, sizeof(out_result->display_name));
    out_result->match_location = to_capi_match_location(result.match_location);
    out_result->match_score = result.match_score;
}

void populate_relationship_search_result(const oep::search::RelationshipSearchResult& result,
                                          oep_relationship_search_result_t* out_result) {
    copy_truncated(result.relationship_id, out_result->relationship_id, sizeof(out_result->relationship_id));
    copy_truncated(result.source_object_id, out_result->source_object_id, sizeof(out_result->source_object_id));
    copy_truncated(result.target_object_id, out_result->target_object_id, sizeof(out_result->target_object_id));
    out_result->relationship_type = to_capi_relationship_type(result.relationship_type);
    out_result->match_location = to_capi_match_location(result.match_location);
    out_result->match_score = result.match_score;
}

} // namespace oep::api::detail

using oep::api::detail::category_for_code;
using oep::api::detail::make_error_result;
using oep::api::detail::make_success_result;
using oep::api::detail::populate_object_info;
using oep::api::detail::populate_object_search_result;
using oep::api::detail::populate_relationship_info;
using oep::api::detail::populate_relationship_search_result;

namespace {

oep_result_t validate_search_arguments(OEP_Runtime runtime, const char* query) {
    if (runtime == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "runtime handle is null");
    }
    if (query == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "query is null");
    }
    if (runtime->runtime.state() != oep::runtime::RuntimeState::RepositoryOpen) {
        return make_error_result(OEP_ERROR_INVALID_STATE, category_for_code(OEP_ERROR_INVALID_STATE),
                                  "no repository is currently open");
    }
    return make_success_result();
}

oep_object_search_result_list_t build_object_search_result_list(const oep::search::SearchObjectsResult& searched) {
    const int count = static_cast<int>(searched.results.size());
    oep_object_search_result_t* items =
        count > 0 ? new oep_object_search_result_t[static_cast<std::size_t>(count)] : nullptr;
    for (int i = 0; i < count; ++i) {
        populate_object_search_result(searched.results[static_cast<std::size_t>(i)], &items[i]);
    }
    oep_object_search_result_list_t list;
    list.items = items;
    list.count = count;
    return list;
}

oep_relationship_search_result_list_t build_relationship_search_result_list(
    const oep::search::SearchRelationshipsResult& searched) {
    const int count = static_cast<int>(searched.results.size());
    oep_relationship_search_result_t* items =
        count > 0 ? new oep_relationship_search_result_t[static_cast<std::size_t>(count)] : nullptr;
    for (int i = 0; i < count; ++i) {
        populate_relationship_search_result(searched.results[static_cast<std::size_t>(i)], &items[i]);
    }
    oep_relationship_search_result_list_t list;
    list.items = items;
    list.count = count;
    return list;
}

} // namespace

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

const char* oep_object_type_to_string(oep_object_type_t type) {
    switch (type) {
        case OEP_OBJECT_TYPE_DOCUMENT: return "Document";
        case OEP_OBJECT_TYPE_DIAGRAM: return "Diagram";
        case OEP_OBJECT_TYPE_COMPONENT: return "Component";
        case OEP_OBJECT_TYPE_PROCEDURE: return "Procedure";
        case OEP_OBJECT_TYPE_PROJECT: return "Project";
        case OEP_OBJECT_TYPE_IMAGE: return "Image";
    }
    return "Document";
}

oep_result_t oep_object_store_get_count(OEP_Runtime runtime, int* out_count) {
    if (out_count != nullptr) {
        *out_count = 0;
    }
    if (runtime == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "runtime handle is null");
    }
    if (out_count == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "out_count is null");
    }
    try {
        if (runtime->runtime.state() != oep::runtime::RuntimeState::RepositoryOpen) {
            return make_error_result(OEP_ERROR_INVALID_STATE, category_for_code(OEP_ERROR_INVALID_STATE),
                                      "no repository is currently open");
        }
        const oep::repository::ListObjectsResult listed = runtime->runtime.object_store()->list_all();
        if (!listed.success) {
            return make_error_result(OEP_ERROR_OPERATION_FAILED, category_for_code(OEP_ERROR_OPERATION_FAILED),
                                      listed.error);
        }
        *out_count = static_cast<int>(listed.objects.size());
        return make_success_result();
    } catch (const std::exception& ex) {
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), ex.what());
    } catch (...) {
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), "unknown internal error");
    }
}

oep_result_t oep_object_store_get_by_id(OEP_Runtime runtime, const char* object_id, oep_object_info_t* out_object) {
    if (out_object != nullptr) {
        zero_object_info(out_object);
    }
    if (runtime == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "runtime handle is null");
    }
    if (object_id == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "object_id is null");
    }
    if (out_object == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "out_object is null");
    }
    try {
        if (runtime->runtime.state() != oep::runtime::RuntimeState::RepositoryOpen) {
            return make_error_result(OEP_ERROR_INVALID_STATE, category_for_code(OEP_ERROR_INVALID_STATE),
                                      "no repository is currently open");
        }
        const oep::repository::LoadObjectResult loaded = runtime->runtime.object_store()->load(object_id);
        if (!loaded.success) {
            return make_error_result(OEP_ERROR_NOT_FOUND, category_for_code(OEP_ERROR_NOT_FOUND), loaded.error);
        }
        populate_object_info(loaded.object, out_object);
        return make_success_result();
    } catch (const std::exception& ex) {
        zero_object_info(out_object);
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), ex.what());
    } catch (...) {
        zero_object_info(out_object);
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), "unknown internal error");
    }
}

oep_result_t oep_object_store_list(OEP_Runtime runtime, oep_object_list_t* out_list) {
    if (out_list != nullptr) {
        zero_object_list(out_list);
    }
    if (runtime == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "runtime handle is null");
    }
    if (out_list == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "out_list is null");
    }
    try {
        if (runtime->runtime.state() != oep::runtime::RuntimeState::RepositoryOpen) {
            return make_error_result(OEP_ERROR_INVALID_STATE, category_for_code(OEP_ERROR_INVALID_STATE),
                                      "no repository is currently open");
        }
        const oep::repository::ListObjectsResult listed = runtime->runtime.object_store()->list_all();
        if (!listed.success) {
            return make_error_result(OEP_ERROR_OPERATION_FAILED, category_for_code(OEP_ERROR_OPERATION_FAILED),
                                      listed.error);
        }

        std::vector<oep::repository::EngineeringObject> objects = listed.objects;
        std::sort(objects.begin(), objects.end(),
                  [](const oep::repository::EngineeringObject& a, const oep::repository::EngineeringObject& b) {
                      return a.object_id < b.object_id;
                  });

        const int count = static_cast<int>(objects.size());
        oep_object_info_t* items = count > 0 ? new oep_object_info_t[static_cast<std::size_t>(count)] : nullptr;
        for (int i = 0; i < count; ++i) {
            populate_object_info(objects[static_cast<std::size_t>(i)], &items[i]);
        }

        out_list->items = items;
        out_list->count = count;
        return make_success_result();
    } catch (const std::exception& ex) {
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), ex.what());
    } catch (...) {
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), "unknown internal error");
    }
}

void oep_object_list_release(oep_object_list_t* list) {
    if (list == nullptr) {
        return;
    }
    delete[] list->items;
    list->items = nullptr;
    list->count = 0;
}

oep_result_t oep_runtime_get_repository_statistics(OEP_Runtime runtime, oep_repository_statistics_t* out_statistics) {
    if (out_statistics != nullptr) {
        zero_statistics(out_statistics);
    }
    if (runtime == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "runtime handle is null");
    }
    if (out_statistics == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "out_statistics is null");
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

        const oep::repository::ListObjectsResult objects_result = runtime->runtime.object_store()->list_all();
        if (!objects_result.success) {
            return make_error_result(OEP_ERROR_OPERATION_FAILED, category_for_code(OEP_ERROR_OPERATION_FAILED),
                                      objects_result.error);
        }

        const oep::repository::ListRelationshipsResult relationships_result =
            runtime->runtime.relationship_store()->list_all();
        if (!relationships_result.success) {
            return make_error_result(OEP_ERROR_OPERATION_FAILED, category_for_code(OEP_ERROR_OPERATION_FAILED),
                                      relationships_result.error);
        }

        const oep::runtime::RuntimePackageSetResult package_result = runtime->runtime.current_package_set();
        if (!package_result.success) {
            return make_error_result(OEP_ERROR_OPERATION_FAILED, category_for_code(OEP_ERROR_OPERATION_FAILED),
                                      package_result.error);
        }

        oep::api::detail::copy_truncated(metadata_result.metadata.repository_id, out_statistics->repository_id,
                                          sizeof(out_statistics->repository_id));
        oep::api::detail::copy_truncated(metadata_result.metadata.repository_name,
                                          out_statistics->repository_name, sizeof(out_statistics->repository_name));
        oep::api::detail::copy_truncated(metadata_result.metadata.repository_version,
                                          out_statistics->repository_version,
                                          sizeof(out_statistics->repository_version));

        out_statistics->total_object_count = static_cast<int>(objects_result.objects.size());
        for (const oep::repository::EngineeringObject& object : objects_result.objects) {
            const int type_index = static_cast<int>(oep::api::detail::to_capi_object_type(object.object_type));
            ++out_statistics->object_count_by_type[type_index];
        }
        out_statistics->relationship_count = static_cast<int>(relationships_result.relationships.size());
        out_statistics->package_count = static_cast<int>(package_result.packages.size());

        return make_success_result();
    } catch (const std::exception& ex) {
        zero_statistics(out_statistics);
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), ex.what());
    } catch (...) {
        zero_statistics(out_statistics);
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), "unknown internal error");
    }
}

const char* oep_relationship_type_to_string(oep_relationship_type_t type) {
    switch (type) {
        case OEP_RELATIONSHIP_TYPE_REFERENCES: return "References";
        case OEP_RELATIONSHIP_TYPE_CONTAINS: return "Contains";
        case OEP_RELATIONSHIP_TYPE_DEPENDS_ON: return "DependsOn";
        case OEP_RELATIONSHIP_TYPE_CONNECTED_TO: return "ConnectedTo";
        case OEP_RELATIONSHIP_TYPE_DOCUMENTS: return "Documents";
        case OEP_RELATIONSHIP_TYPE_IMPLEMENTS: return "Implements";
    }
    return "References";
}

oep_result_t oep_relationship_store_get_count(OEP_Runtime runtime, int* out_count) {
    if (out_count != nullptr) {
        *out_count = 0;
    }
    if (runtime == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "runtime handle is null");
    }
    if (out_count == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "out_count is null");
    }
    try {
        if (runtime->runtime.state() != oep::runtime::RuntimeState::RepositoryOpen) {
            return make_error_result(OEP_ERROR_INVALID_STATE, category_for_code(OEP_ERROR_INVALID_STATE),
                                      "no repository is currently open");
        }
        const oep::repository::ListRelationshipsResult listed = runtime->runtime.relationship_store()->list_all();
        if (!listed.success) {
            return make_error_result(OEP_ERROR_OPERATION_FAILED, category_for_code(OEP_ERROR_OPERATION_FAILED),
                                      listed.error);
        }
        *out_count = static_cast<int>(listed.relationships.size());
        return make_success_result();
    } catch (const std::exception& ex) {
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), ex.what());
    } catch (...) {
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), "unknown internal error");
    }
}

oep_result_t oep_relationship_store_get_by_id(OEP_Runtime runtime, const char* relationship_id,
                                               oep_relationship_info_t* out_relationship) {
    if (out_relationship != nullptr) {
        zero_relationship_info(out_relationship);
    }
    if (runtime == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "runtime handle is null");
    }
    if (relationship_id == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "relationship_id is null");
    }
    if (out_relationship == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "out_relationship is null");
    }
    try {
        if (runtime->runtime.state() != oep::runtime::RuntimeState::RepositoryOpen) {
            return make_error_result(OEP_ERROR_INVALID_STATE, category_for_code(OEP_ERROR_INVALID_STATE),
                                      "no repository is currently open");
        }
        const oep::repository::LoadRelationshipResult loaded =
            runtime->runtime.relationship_store()->load(relationship_id);
        if (!loaded.success) {
            return make_error_result(OEP_ERROR_NOT_FOUND, category_for_code(OEP_ERROR_NOT_FOUND), loaded.error);
        }
        populate_relationship_info(loaded.relationship, out_relationship);
        return make_success_result();
    } catch (const std::exception& ex) {
        zero_relationship_info(out_relationship);
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), ex.what());
    } catch (...) {
        zero_relationship_info(out_relationship);
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), "unknown internal error");
    }
}

oep_result_t oep_relationship_store_list(OEP_Runtime runtime, oep_relationship_list_t* out_list) {
    if (out_list != nullptr) {
        zero_relationship_list(out_list);
    }
    if (runtime == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "runtime handle is null");
    }
    if (out_list == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "out_list is null");
    }
    try {
        if (runtime->runtime.state() != oep::runtime::RuntimeState::RepositoryOpen) {
            return make_error_result(OEP_ERROR_INVALID_STATE, category_for_code(OEP_ERROR_INVALID_STATE),
                                      "no repository is currently open");
        }
        const oep::repository::ListRelationshipsResult listed = runtime->runtime.relationship_store()->list_all();
        if (!listed.success) {
            return make_error_result(OEP_ERROR_OPERATION_FAILED, category_for_code(OEP_ERROR_OPERATION_FAILED),
                                      listed.error);
        }

        std::vector<oep::repository::Relationship> relationships = listed.relationships;
        std::sort(relationships.begin(), relationships.end(),
                  [](const oep::repository::Relationship& a, const oep::repository::Relationship& b) {
                      return a.relationship_id < b.relationship_id;
                  });

        const int count = static_cast<int>(relationships.size());
        oep_relationship_info_t* items =
            count > 0 ? new oep_relationship_info_t[static_cast<std::size_t>(count)] : nullptr;
        for (int i = 0; i < count; ++i) {
            populate_relationship_info(relationships[static_cast<std::size_t>(i)], &items[i]);
        }

        out_list->items = items;
        out_list->count = count;
        return make_success_result();
    } catch (const std::exception& ex) {
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), ex.what());
    } catch (...) {
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), "unknown internal error");
    }
}

void oep_relationship_list_release(oep_relationship_list_t* list) {
    if (list == nullptr) {
        return;
    }
    delete[] list->items;
    list->items = nullptr;
    list->count = 0;
}

const char* oep_match_location_to_string(oep_match_location_t location) {
    switch (location) {
        case OEP_MATCH_LOCATION_NAME: return "Name";
        case OEP_MATCH_LOCATION_DESCRIPTION: return "Description";
        case OEP_MATCH_LOCATION_AUTHOR: return "Author";
        case OEP_MATCH_LOCATION_TAGS: return "Tags";
        case OEP_MATCH_LOCATION_OBJECT_TYPE: return "ObjectType";
        case OEP_MATCH_LOCATION_RELATIONSHIP_TYPE: return "RelationshipType";
    }
    return "Name";
}

oep_result_t oep_search_repository(OEP_Runtime runtime, const char* query,
                                    oep_repository_search_result_t* out_result) {
    if (out_result != nullptr) {
        zero_repository_search_result(out_result);
    }
    const oep_result_t argument_check = validate_search_arguments(runtime, query);
    if (!argument_check.success) {
        return argument_check;
    }
    if (out_result == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "out_result is null");
    }
    try {
        const oep::search::SearchObjectsResult objects_searched = runtime->runtime.search_engine()->search_objects(query);
        if (!objects_searched.success) {
            return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                      objects_searched.error);
        }
        const oep::search::SearchRelationshipsResult relationships_searched =
            runtime->runtime.search_engine()->search_relationships(query);
        if (!relationships_searched.success) {
            return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                      relationships_searched.error);
        }

        out_result->objects = build_object_search_result_list(objects_searched);
        out_result->relationships = build_relationship_search_result_list(relationships_searched);
        return make_success_result();
    } catch (const std::exception& ex) {
        zero_repository_search_result(out_result);
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), ex.what());
    } catch (...) {
        zero_repository_search_result(out_result);
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), "unknown internal error");
    }
}

void oep_repository_search_result_release(oep_repository_search_result_t* result) {
    if (result == nullptr) {
        return;
    }
    delete[] result->objects.items;
    result->objects.items = nullptr;
    result->objects.count = 0;
    delete[] result->relationships.items;
    result->relationships.items = nullptr;
    result->relationships.count = 0;
}

oep_result_t oep_search_objects(OEP_Runtime runtime, const char* query, oep_object_search_result_list_t* out_list) {
    if (out_list != nullptr) {
        zero_object_search_result_list(out_list);
    }
    const oep_result_t argument_check = validate_search_arguments(runtime, query);
    if (!argument_check.success) {
        return argument_check;
    }
    if (out_list == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "out_list is null");
    }
    try {
        const oep::search::SearchObjectsResult searched = runtime->runtime.search_engine()->search_objects(query);
        if (!searched.success) {
            return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                      searched.error);
        }
        *out_list = build_object_search_result_list(searched);
        return make_success_result();
    } catch (const std::exception& ex) {
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), ex.what());
    } catch (...) {
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), "unknown internal error");
    }
}

void oep_object_search_result_list_release(oep_object_search_result_list_t* list) {
    if (list == nullptr) {
        return;
    }
    delete[] list->items;
    list->items = nullptr;
    list->count = 0;
}

oep_result_t oep_search_relationships(OEP_Runtime runtime, const char* query,
                                      oep_relationship_search_result_list_t* out_list) {
    if (out_list != nullptr) {
        zero_relationship_search_result_list(out_list);
    }
    const oep_result_t argument_check = validate_search_arguments(runtime, query);
    if (!argument_check.success) {
        return argument_check;
    }
    if (out_list == nullptr) {
        return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                  "out_list is null");
    }
    try {
        const oep::search::SearchRelationshipsResult searched =
            runtime->runtime.search_engine()->search_relationships(query);
        if (!searched.success) {
            return make_error_result(OEP_ERROR_INVALID_ARGUMENT, category_for_code(OEP_ERROR_INVALID_ARGUMENT),
                                      searched.error);
        }
        *out_list = build_relationship_search_result_list(searched);
        return make_success_result();
    } catch (const std::exception& ex) {
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), ex.what());
    } catch (...) {
        return make_error_result(OEP_ERROR_INTERNAL, category_for_code(OEP_ERROR_INTERNAL), "unknown internal error");
    }
}

void oep_relationship_search_result_list_release(oep_relationship_search_result_list_t* list) {
    if (list == nullptr) {
        return;
    }
    delete[] list->items;
    list->items = nullptr;
    list->count = 0;
}

} // extern "C"
