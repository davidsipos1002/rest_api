#pragma once

#include <RestController.hpp>
#include <Database.hpp>
#include <Json.hpp>

template <TableName T, EntityConcept E>
RestController::Response createUpdate(Database &database, const RestController::Request &request)
{
    std::optional<E> optional;
    Json::parse(request.second, optional);
    std::string reponseBody;
    if (!optional.has_value())
    {
        reponseBody = Json::status<false>();
        std::cout << "not parsed" << std::endl;
    }
    else
    {
        E &entity = optional.value();
        int rows = 0;
        if (request.first.second.contains("/create"))
            rows = database.create<T>(entity);
        else
            rows = database.update<T>(entity);
        if (rows)
            reponseBody = Json::toJson(entity);
        else
            reponseBody = Json::status<false>();
    }
    return std::make_pair("200 OK", reponseBody);
}

template <TableName T, EntityConcept E>
RestController::Response fetchAll(Database &database, const RestController::Request &request)
{
    std::vector<E> all;
    database.fetchAll<T>(all);
    std::string allJson = Json::toJson(all);
    return std::make_pair("200 OK", allJson);
}

template <TableName T, EntityConcept E>
RestController::Response idOperation(Database &database, const RestController::Request &request)
{
    std::optional<std::string> optionalId = Json::parseId(request.second);
    std::string responseBody;
    if (optionalId.has_value())
    {
        std::string &id = optionalId.value();
        if (request.first.second.contains("/delete"))
        {
            if (database.remove<T>(id) > 0)
                responseBody = Json::status<true>();
            else
                responseBody = Json::status<false>();
        }
        else
        {
            std::optional<E> optionalEntity;
            database.fetchById<T>(id, optionalEntity);
            if (optionalEntity.has_value())
                responseBody = Json::toJson(optionalEntity.value());
            else
                responseBody = Json::status<false>();
        }
    }
    else
        responseBody = Json::status<false>();
    return std::make_pair("200 OK", responseBody);
}

static inline void registerHandlers(RestController &controller)
{
    controller.registerEndpoint(RestController::HttpMethod::POST, "/books/create",
                                createUpdate<Entities::Book::BookTable, Entities::Book::BookEntity>);
    controller.registerEndpoint(RestController::HttpMethod::POST, "/stock/create",
                                createUpdate<Entities::Stock::StockTable, Entities::Stock::StockEntity>);

    controller.registerEndpoint(RestController::HttpMethod::POST, "/books/update",
                                createUpdate<Entities::Book::BookTable, Entities::Book::BookEntity>);
    controller.registerEndpoint(RestController::HttpMethod::POST, "/stock/update",
                                createUpdate<Entities::Stock::StockTable, Entities::Stock::StockEntity>);

    controller.registerEndpoint(RestController::HttpMethod::GET, "/books/fetchAll",
                                fetchAll<Entities::Book::BookTable, Entities::Book::BookEntity>);
    controller.registerEndpoint(RestController::HttpMethod::GET, "/stock/fetchAll",
                                fetchAll<Entities::Stock::StockTable, Entities::Stock::StockEntity>);

    controller.registerEndpoint(RestController::HttpMethod::POST, "/books/delete",
                                idOperation<Entities::Book::BookTable, Entities::Book::BookEntity>);
    controller.registerEndpoint(RestController::HttpMethod::POST, "/stock/delete",
                                idOperation<Entities::Stock::StockTable, Entities::Stock::StockEntity>);

    controller.registerEndpoint(RestController::HttpMethod::POST, "/books/fetchById",
                                idOperation<Entities::Book::BookTable, Entities::Book::BookEntity>);

    controller.registerEndpoint(RestController::HttpMethod::POST, "/stock/fetchById",
                                idOperation<Entities::Stock::StockTable, Entities::Stock::StockEntity>);
}
