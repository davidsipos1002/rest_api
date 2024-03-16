#include <Database.hpp>

#include <iostream>

Database::Database() : session(mysqlx::Session("localhost", 33060, "david", "david12345678")),
                       schema(session.getSchema("books")) {}

Database::~Database()
{
    session.close();
}

std::string Database::generateUuid()
{
    std::random_device rd;
    auto seed_data = std::array<int, std::mt19937::state_size>{};
    std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
    std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
    std::mt19937 generator(seq);

    return uuids::to_string(uuids::uuid_random_generator{generator}());
}
