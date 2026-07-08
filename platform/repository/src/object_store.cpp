#include "oep/repository/object_store.hpp"

#include <fstream>
#include <sstream>
#include <system_error>

#include "json_value.hpp"
#include "oep/repository/timestamp.hpp"
#include "oep/repository/uuid.hpp"

namespace oep::repository {

namespace {

std::string join_errors(const std::vector<std::string>& errors) {
    std::string message;
    for (std::size_t i = 0; i < errors.size(); ++i) {
        message += errors[i];
        if (i + 1 != errors.size()) message += "; ";
    }
    return message;
}

json::Value to_json(const EngineeringObject& object) {
    json::Array tags;
    for (const std::string& tag : object.tags) {
        tags.push_back(json::Value::string(tag));
    }

    json::Object fields;
    fields.emplace_back("objectId", json::Value::string(object.object_id));
    fields.emplace_back("objectType", json::Value::string(to_string(object.object_type)));
    fields.emplace_back("name", json::Value::string(object.name));
    fields.emplace_back("description", json::Value::string(object.description));
    fields.emplace_back("createdUtc", json::Value::string(object.created_utc));
    fields.emplace_back("lastModifiedUtc", json::Value::string(object.last_modified_utc));
    fields.emplace_back("version", json::Value::string(object.version));
    fields.emplace_back("author", json::Value::string(object.author));
    fields.emplace_back("tags", json::Value::array(std::move(tags)));
    return json::Value::object(std::move(fields));
}

std::string read_string_field(const json::Value& value, const char* key) {
    const json::Value* found = value.find(key);
    return (found != nullptr && found->is_string()) ? found->as_string() : std::string();
}

std::vector<std::string> read_tags(const json::Value& value) {
    std::vector<std::string> tags;
    const json::Value* found = value.find("tags");
    if (found == nullptr || !found->is_array()) {
        return tags;
    }
    for (const json::Value& entry : found->as_array()) {
        if (entry.is_string()) {
            tags.push_back(entry.as_string());
        }
    }
    return tags;
}

// Returns {success, error, object}. Does not validate the result.
struct ParsedObject {
    bool success = false;
    std::string error;
    EngineeringObject object;
};

ParsedObject read_object_file(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {false, "could not open '" + path.string() + "'", {}};
    }

    std::ostringstream contents_stream;
    contents_stream << file.rdbuf();

    const json::ParseResult parsed = json::parse(contents_stream.str());
    if (!parsed.success) {
        return {false, "invalid JSON in '" + path.string() + "': " + parsed.error, {}};
    }
    if (!parsed.value.is_object()) {
        return {false, "'" + path.string() + "' must contain a JSON object", {}};
    }

    EngineeringObject object;
    object.object_id = read_string_field(parsed.value, "objectId");

    const std::string type_name = read_string_field(parsed.value, "objectType");
    const std::optional<ObjectType> type = object_type_from_string(type_name);
    if (!type.has_value()) {
        return {false, "'" + path.string() + "' has an unrecognized objectType '" + type_name + "'", {}};
    }
    object.object_type = *type;

    object.name = read_string_field(parsed.value, "name");
    object.description = read_string_field(parsed.value, "description");
    object.created_utc = read_string_field(parsed.value, "createdUtc");
    object.last_modified_utc = read_string_field(parsed.value, "lastModifiedUtc");
    object.version = read_string_field(parsed.value, "version");
    object.author = read_string_field(parsed.value, "author");
    object.tags = read_tags(parsed.value);

    return {true, "", object};
}

ObjectResult write_object_file(const std::filesystem::path& path, const EngineeringObject& object) {
    const std::string serialized = json::serialize(to_json(object));

    const std::filesystem::path temp_path = path.string() + ".tmp";
    {
        std::ofstream file(temp_path, std::ios::binary | std::ios::trunc);
        if (!file) {
            return {false, "could not open '" + temp_path.string() + "' for writing"};
        }
        file << serialized;
    }

    std::error_code error_code;
    std::filesystem::rename(temp_path, path, error_code);
    if (error_code) {
        std::filesystem::remove(temp_path, error_code);
        return {false, "could not finalize '" + path.string() + "': " + error_code.message()};
    }

    return {true, ""};
}

} // namespace

ObjectStore::ObjectStore(std::filesystem::path root) : root_(std::move(root)) {}

std::filesystem::path ObjectStore::path_for(const std::string& object_id) const {
    return root_ / (object_id + ".json");
}

LoadObjectResult ObjectStore::create(EngineeringObject object) const {
    if (object.object_id.empty()) {
        object.object_id = generate_uuid_v4();
    }

    const std::string now = current_timestamp_utc();
    if (object.created_utc.empty()) {
        object.created_utc = now;
    }
    object.last_modified_utc = now;

    const std::vector<std::string> errors = validate_object(object);
    if (!errors.empty()) {
        return {false, "refusing to create invalid object: " + join_errors(errors), {}};
    }

    std::error_code error_code;
    std::filesystem::create_directories(root_, error_code);
    if (error_code) {
        return {false, "could not create '" + root_.string() + "': " + error_code.message(), {}};
    }

    const std::filesystem::path path = path_for(object.object_id);
    if (std::filesystem::exists(path)) {
        return {false, "an object with id '" + object.object_id + "' already exists", {}};
    }

    const ObjectResult write_result = write_object_file(path, object);
    if (!write_result.success) {
        return {false, write_result.error, {}};
    }

    return {true, "", object};
}

LoadObjectResult ObjectStore::load(const std::string& object_id) const {
    const std::filesystem::path path = path_for(object_id);
    if (!std::filesystem::exists(path)) {
        return {false, "no object with id '" + object_id + "'", {}};
    }

    const ParsedObject parsed = read_object_file(path);
    if (!parsed.success) {
        return {false, parsed.error, {}};
    }

    const std::vector<std::string> errors = validate_object(parsed.object);
    if (!errors.empty()) {
        return {false, "object '" + object_id + "' failed validation: " + join_errors(errors), {}};
    }

    return {true, "", parsed.object};
}

ObjectResult ObjectStore::update(EngineeringObject object) const {
    const std::filesystem::path path = path_for(object.object_id);
    if (!std::filesystem::exists(path)) {
        return {false, "no object with id '" + object.object_id + "'"};
    }

    const ParsedObject existing = read_object_file(path);
    if (!existing.success) {
        return {false, existing.error};
    }

    // objectId, objectType, and createdUtc are immutable.
    object.object_type = existing.object.object_type;
    object.created_utc = existing.object.created_utc;
    object.last_modified_utc = current_timestamp_utc();

    const std::vector<std::string> errors = validate_object(object);
    if (!errors.empty()) {
        return {false, "refusing to save invalid object: " + join_errors(errors)};
    }

    return write_object_file(path, object);
}

ObjectResult ObjectStore::remove(const std::string& object_id) const {
    const std::filesystem::path path = path_for(object_id);
    if (!std::filesystem::exists(path)) {
        return {false, "no object with id '" + object_id + "'"};
    }

    std::error_code error_code;
    std::filesystem::remove(path, error_code);
    if (error_code) {
        return {false, "could not remove '" + path.string() + "': " + error_code.message()};
    }

    return {true, ""};
}

ListObjectsResult ObjectStore::list_all() const {
    std::vector<EngineeringObject> objects;

    std::error_code error_code;
    if (!std::filesystem::exists(root_, error_code)) {
        return {true, "", objects};
    }

    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(root_, error_code)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json") {
            continue;
        }
        const ParsedObject parsed = read_object_file(entry.path());
        if (!parsed.success) {
            continue; // skip files that are not valid Engineering Objects
        }
        objects.push_back(parsed.object);
    }

    if (error_code) {
        return {false, "could not enumerate '" + root_.string() + "': " + error_code.message(), {}};
    }

    return {true, "", objects};
}

} // namespace oep::repository
