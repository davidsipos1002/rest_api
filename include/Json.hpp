#pragma once

#include <optional>
#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <sstream>

#include <Field.hpp>
#include <Entity.hpp>

class Json
{
private:
    template <std::size_t i, FieldConcept... Fields>
    struct ToJson
    {
        using FieldValueType = GetFieldType<i, Fields...>::type;

        void operator()(rapidjson::Writer<rapidjson::OStreamWrapper> &writer, const Entity<Fields...> &entity)
        {
            ToJson<i - 1, Fields...>{}(writer, entity);
            writer.Key(getField<i, Fields...>(entity).columnName.string);
            if constexpr (std::is_same_v<std::string, FieldValueType>)
                writer.String(getField<i, Fields...>(entity).value.c_str(), getField<i, Fields...>(entity).value.size());
            else if (std::is_same_v<int, FieldValueType>)
                writer.Int(getField<i, Fields...>(entity).value);
            else
                writer.Double(getField<i, Fields...>(entity).value);
        }
    };

    template <FieldConcept... Fields>
    struct ToJson<0, Fields...>
    {
        using FieldValueType = GetFieldType<0, Fields...>::type;

        void operator()(rapidjson::Writer<rapidjson::OStreamWrapper> &writer, const Entity<Fields...> &entity)
        {
            writer.Key(getField<0, Fields...>(entity).columnName.string);
            if constexpr (std::is_same_v<std::string, FieldValueType>)
                writer.String(getField<0, Fields...>(entity).value.c_str(), getField<0, Fields...>(entity).value.size());
            else if (std::is_same_v<int, FieldValueType>)
                writer.Int(getField<0, Fields...>(entity).value);
            else
                writer.Double(getField<0, Fields...>(entity).value);
        }
    };

    template <std::size_t i, FieldConcept... Fields>
    struct ParseJson
    {
        using FieldValueType = GetFieldType<i, Fields...>::type;

        bool operator()(const rapidjson::Document &doc, Entity<Fields...> &entity)
        {
            bool ok = ParseJson<i - 1, Fields...>{}(doc, entity);
            if (ok)
            {
                const char *columnName = getField<i, Fields...>(entity).columnName.string;
                if constexpr (std::is_same_v<std::string, FieldValueType>)
                {
                    if (doc.HasMember(columnName) && doc[columnName].IsString())
                        getField<i>(entity).value = doc[columnName].GetString();
                    else
                        return false;
                }
                else if (std::is_same_v<int, FieldValueType>)
                {
                    if (doc.HasMember(columnName) && doc[columnName].IsInt())
                        getField<i>(entity).value = doc[columnName].GetInt();
                    else
                        return false;
                }
                else
                {
                    if (doc.HasMember(columnName) && doc[columnName].IsDouble())
                        getField<i>(entity).value = doc[columnName].GetDouble();
                    else
                        return false;
                }
                return true;
            }
            return false;
        }
    };

    template <FieldConcept... Fields>
    struct ParseJson<0, Fields...>
    {
        using FieldValueType = GetFieldType<0, Fields...>::type;

        bool operator()(const rapidjson::Document &doc, Entity<Fields...> &entity)
        {
            const char *columnName = getField<0, Fields...>(entity).columnName.string;
            if constexpr (std::is_same_v<std::string, FieldValueType>)
            {
                if (doc.HasMember(columnName) && doc[columnName].IsString())
                    getField<0>(entity).value = doc[columnName].GetString();
                else
                    return false;
            }
            else if (std::is_same_v<int, FieldValueType>)
            {
                if (doc.HasMember(columnName) && doc[columnName].IsInt())
                    getField<0>(entity).value = doc[columnName].GetInt();
                else
                    return false;
            }
            else
            {
                if (doc.HasMember(columnName) && doc[columnName].IsDouble())
                    getField<0>(entity).value = doc[columnName].GetDouble();
                else
                    return false;
            }
            return true;
        }
    };

public:
    template <bool S = true, FieldConcept... Fields>
    static inline std::string toJson(const Entity<Fields...> &entity)
    {
        std::ostringstream sstream;
        rapidjson::OStreamWrapper out(sstream);
        rapidjson::Writer<rapidjson::OStreamWrapper> writer(out);
        writer.StartObject();
        if (S)
        {
            writer.Key("success");
            writer.Bool(true);
        }
        ToJson<sizeof...(Fields) - 1, Fields...>{}(writer, entity);
        writer.EndObject();
        return sstream.str();
    }

    template <FieldConcept... Fields>
    static inline std::string toJson(const std::vector<Entity<Fields...>> &entities)
    {
        std::ostringstream sstream;
        rapidjson::OStreamWrapper out(sstream);
        rapidjson::Writer<rapidjson::OStreamWrapper> writer(out);
        writer.StartObject();
        writer.Key("success");
        writer.Bool(true);
        writer.Key("size");
        writer.Int(entities.size());
        writer.Key("entities");
        writer.StartArray();
        for (const auto &e : entities)
        {
            std::string json = toJson<false>(e);
            writer.RawValue(json.c_str(), json.size(), rapidjson::kObjectType);
        }
        writer.EndArray();
        writer.EndObject();
        return sstream.str();
    }

    template <bool S = true>
    static inline std::string status()
    {   
        std::ostringstream sstream;
        rapidjson::OStreamWrapper out(sstream);
        rapidjson::Writer<rapidjson::OStreamWrapper> writer(out);
        writer.StartObject();
        writer.Key("success");
        writer.Bool(S);
        writer.EndObject();
        return sstream.str();
    }

    static std::optional<std::string> parseId(const std::string &json);

    template <FieldConcept... Fields>
    static inline void parse(const std::string &json, std::optional<Entity<Fields...>> &entityOptional)
    {
        entityOptional = {};
        rapidjson::Document doc;
        doc.Parse(json.c_str());
        if (doc.IsNull())
            return;
        Entity<Fields...> entity;
        if (ParseJson<sizeof...(Fields) - 1, Fields...>{}(doc, entity))
            entityOptional = entity;
    }
};
