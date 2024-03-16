#include <Json.hpp>

std::optional<std::string> Json::parseId(const std::string &json)
{
    std::string id;
    rapidjson::Document doc;
    doc.Parse(json.c_str()); 
    if (doc.IsNull())
        return {};
    if (doc.HasMember("id") && doc["id"].IsString())
        return doc["id"].GetString();
    else
        return {};
}
