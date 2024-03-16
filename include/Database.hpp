#pragma once

#include <mysqlx/xdevapi.h>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <uuid.h>

#include <Field.hpp>
#include <Entity.hpp>
class Database
{
private:
    mysqlx::Session session;
    mysqlx::Schema schema;

    std::string generateUuid();

    template <std::size_t i, FieldConcept... Fields>
    struct GetTableUpdate
    {
        mysqlx::TableUpdate operator()(mysqlx::Table &t, const Entity<Fields...> &entity)
        {
            return GetTableUpdate<i - 1, Fields...>{}(t, entity).set(getField<i, Fields...>(entity).columnName.string,
                                                                     getField<i, Fields...>(entity).value);
        }
    };

    template <FieldConcept... Fields>
    struct GetTableUpdate<0, Fields...>
    {
        mysqlx::TableUpdate operator()(mysqlx::Table &t, const Entity<Fields...> &entity)
        {
            return t.update();
        }
    };

    template <std::size_t i, FieldConcept... Fields>
    struct FillEntity
    {
        using FieldValueType = GetFieldType<i, Fields...>::type;

        void operator()(const mysqlx::Row &row, Entity<Fields...> &entity)
        {
            FillEntity<i - 1, Fields...>{}(row, entity);
            getField<i, Fields...>(entity).value = FieldValueType(row[i]);
        }
    };

    template <FieldConcept... Fields>
    struct FillEntity<0, Fields...>
    {
        using FieldValueType = GetFieldType<0, Fields...>::type;

        void operator()(const mysqlx::Row &row, Entity<Fields...> &entity)
        {
            getField<0, Fields...>(entity).value = FieldValueType(row[0]);
        }
    };

    template <TableName T, FieldConcept... Fields, std::size_t... I>
    inline int createImpl(Entity<Fields...> &entity, std::index_sequence<I...>)
    {
        std::string uuid = generateUuid();
        getField<0>(entity) = Field<GetColumnName<0, Fields...>::name.string, typename GetFieldType<0, Fields...>::type>(uuid);
        mysqlx::Table t = schema.getTable(T.string);
        mysqlx::Result res = t.insert(Fields::columnName.string...)
                                 .values(getField<I, Fields...>(entity).value...)
                                 .execute();
        return res.getAffectedItemsCount();
    }

public:
    Database();
    ~Database();

    template <TableName T, FieldConcept... Fields, typename Indices = std::make_index_sequence<sizeof...(Fields)>>
    inline int create(Entity<Fields...> &entity)
    {
        return createImpl<T>(entity, Indices{});
    }

    template <TableName T, FieldConcept... Fields>
    inline int update(const Entity<Fields...> &entity)
    {
        mysqlx::Table t = schema.getTable(T.string);
        mysqlx::Result res = GetTableUpdate<sizeof...(Fields) - 1, Fields...>{}(t, entity)
                                 .where("id LIKE :uuid")
                                 .bind("uuid", getField<0>(entity).value)
                                 .execute();
        return res.getAffectedItemsCount();
    }

    template <TableName T>
    inline int remove(const std::string &id)
    {
        mysqlx::Table t = schema.getTable(T.string);
        mysqlx::Result res = t.remove()
                                 .where("id LIKE :uuid")
                                 .bind("uuid", id)
                                 .execute();
        return res.getAffectedItemsCount();
    }

    template <TableName T, FieldConcept... Fields>
    inline void fetchAll(std::vector<Entity<Fields...>> &entities)
    {
        mysqlx::Table t = schema.getTable(T.string);
        mysqlx::RowResult result = t.select().execute();
        for (const auto &row : result)
        {
            Entity<Fields...> entity;
            FillEntity<sizeof...(Fields) - 1, Fields...>{}(row, entity);
            entities.push_back(entity);
        }
    }

    template <TableName T, FieldConcept... Fields>
    inline void fetchById(const std::string &id, std::optional<Entity<Fields...>> &entityOptional)
    {
        mysqlx::Table t = schema.getTable(T.string);
        mysqlx::RowResult result = t.select()
                                       .where("id LIKE :uuid")
                                       .bind("uuid", id)
                                       .execute();
        if (!result.count())
        {
            entityOptional = {};
            return;
        }
        mysqlx::Row row = result.fetchOne();
        Entity<Fields...> entity;
        FillEntity<sizeof...(Fields) - 1, Fields...>{}(row, entity);
        entityOptional = entity;
    }
};

namespace Entities
{
    namespace Book
    {
        using IdField = Field<"id", std::string>;
        using AuthorField = Field<"author", std::string>;
        using TitleField = Field<"title", std::string>;
        using GenreField = Field<"genre", std::string>;
        using PublisherField = Field<"publisher", std::string>;
        using BookEntity = Entity<IdField, AuthorField, TitleField, GenreField, PublisherField>;
        static inline constexpr TableName BookTable = "book";
    };

    namespace Stock
    {
        using IdField = Field<"id", std::string>;
        using BookField = Field<"book", std::string>;
        using PriceField = Field<"price", double>;
        using CountField = Field<"count", int>;
        using StockEntity = Entity<IdField, BookField, PriceField, CountField>;
        static inline constexpr TableName StockTable = "stock";
    }
};