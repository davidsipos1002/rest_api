#pragma once

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <functional>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

#include <Database.hpp>
#include <Log.hpp>
#include <ThreadPool.hpp>

class RestController
{
public:
    enum class HttpMethod
    {
        GET,
        POST
    };

    using Endpoint = std::pair<HttpMethod, std::string>;
    using Request = std::pair<Endpoint, std::string>;
    using Response = std::pair<std::string, std::string>;
    using EndpointHandler = std::function<Response(Database &database, const Request &)>;

private:
    bool running;
    int socketFd;
    int port;
    std::thread listener;
    ThreadPool<int> workerPool;
    std::map<Endpoint, EndpointHandler> endpoints;
    Database database;

    std::optional<Request> parseRequest(const char *requestBuffer);
    void handleClient(int clientSocket);
    void acceptConnections();

public:
    explicit RestController(int threadCount = 10, int port = 8080);

    ~RestController();

    RestController(const RestController &) = delete;
    RestController(RestController &&) = delete;
    RestController &operator=(const RestController &) = delete;
    RestController &operator=(RestController &&) = delete;

    void registerEndpoint(HttpMethod method, const std::string &endpoint, const EndpointHandler &handler);
    void startController();
    void stopController();
};

template <>
struct ClassName<RestController>
{
    static constexpr const char *name = "RestController";
};
