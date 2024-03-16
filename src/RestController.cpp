#include <RestController.hpp>

RestController::RestController(int threadCount, int port) : workerPool(threadCount), port(port) {}

RestController::~RestController()
{
    stopController();
}

std::optional<RestController::Request> RestController::parseRequest(const char *requestBuffer)
{
    std::istringstream in(requestBuffer);

    std::string line;
    if (!std::getline(in, line))
        return {};

    std::istringstream stream(line);
    std::string endpoint;

    stream >> endpoint;
    HttpMethod method;
    if (!endpoint.compare("POST"))
        method = HttpMethod::POST;
    else if (!endpoint.compare("GET"))
        method = HttpMethod::GET;
    else
        return {};

    stream >> endpoint;

    std::string content;
    if (method == HttpMethod::POST)
    {
        size_t contentLength = 0;
        bool isJson = false;
        std::string buffer;
        while (std::getline(in, line))
        {
            stream = std::istringstream(line);
            stream >> buffer;
            std::transform(buffer.begin(), buffer.end(), buffer.begin(), [](unsigned char c)
                           { return std::tolower(c); });
            if (!buffer.compare("content-type:"))
            {
                stream >> buffer;
                if (!buffer.compare("application/json"))
                    isJson = true;
            }
            else if (!buffer.compare("content-length:"))
                stream >> contentLength;
        }
        if (isJson && contentLength)
            content = line;
    }

    Endpoint endPoint = std::make_pair(method, endpoint);
    return std::make_pair(endPoint, content);
}

void RestController::handleClient(int clientSocket)
{
    static const char *notFoundResponse = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    static const size_t notFoundResponseLength = strlen(notFoundResponse);

    char *requestBuffer = new char[4096];
    ssize_t size = 4096;
    char *currentPosition = requestBuffer;
    struct pollfd pollFd;
    memset(&pollFd, 0, sizeof(struct pollfd));

    pollFd.fd = clientSocket;
    pollFd.events = POLLIN;
    poll(&pollFd, 1, 100);

    while (pollFd.revents & POLLIN)
    {
        ssize_t ret = recv(clientSocket, currentPosition, size, 0);
        if (ret < 0)
            break;
        size -= ret;
        currentPosition += ret;
        pollFd.revents = 0;
        poll(&pollFd, 1, 100);
    }
    *currentPosition = 0;

    if (size == 4096)
    {
        delete[] requestBuffer;
        close(clientSocket);
        return;
    }

    std::optional<Request> requestOptional = parseRequest(requestBuffer);
    delete[] requestBuffer;

    bool requestServiced = false;
    if (requestOptional.has_value())
    {
        Request request = requestOptional.value();
        std::string message = "Method: ";
        message += request.first.first == HttpMethod::POST ? "POST " : "GET ";
        message += "Endpoint: ";
        message += request.first.second;
        if (request.second.size())
            message += " Content: " + request.second;
        log<RestController>(message);
        std::map<Endpoint, EndpointHandler>::const_iterator it;
        if ((it = endpoints.find(request.first)) != endpoints.cend())
        {
            log<RestController>("Servicing request...");
            requestServiced = true;
            Response response = it->second(database, request);
            std::ostringstream out;
            out << "HTTP/1.1 " << response.first << "\r\n";
            out << "Content-Type: \"application/json\"\r\n";
            out << "Content-Length: " << response.second.size() << "\r\n";
            out << "\r\n";
            out << response.second << "\r\n";
            std::string responseString = out.str();
            send(clientSocket, responseString.c_str(), responseString.size(), 0);
            log<RestController>("Request serviced.");
        }
    }

    if (!requestServiced)
    {
        log<RestController>("Unknown request.");
        send(clientSocket, notFoundResponse, notFoundResponseLength, 0);
    }

    close(clientSocket);
}

void RestController::acceptConnections()
{
    if (listen(socketFd, 100) < 0)
    {
        log<RestController>("Could not start listening.");
        running = false;
        return;
    }
    int clientSock;
    struct sockaddr_in clientAddr;
    socklen_t length;
    log<RestController>("Listening for clients...");
    while (running)
    {
        if ((clientSock = accept(socketFd, (struct sockaddr *)&clientAddr, &length)) < 0)
            continue;

        log<RestController>("Client accepted. Forwarding to thread pool...");
        ThreadPool<int>::Work poolHandler = std::bind(&RestController::handleClient, this, std::placeholders::_1);
        workerPool.addTask(poolHandler, clientSock);
    }
}

void RestController::registerEndpoint(HttpMethod method, const std::string &endpoint, const EndpointHandler &handler)
{
    if (running)
        return;
    Endpoint key = std::make_pair(method, endpoint);
    endpoints[key] = handler;
}

void RestController::startController()
{
    log<RestController>("Starting controller...");
    socketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketFd < 0)
    {
        log<RestController>("Could not create socket.");
        return;
    }
    int socketFlags = fcntl(socketFd, F_GETFL, 0);
    if (socketFlags < 0)
    {
        log<RestController>("Could not get socket flags.");
        return;
    }
    if (fcntl(socketFd, F_SETFL, socketFlags | O_NONBLOCK) < 0)
    {
        log<RestController>("Could not set socket flags.");
        return;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(socketFd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
    {
        log<RestController>("Could not bind socket.");
        return;
    }
    running = true;
    listener = std::thread(&RestController::acceptConnections, this);
}

void RestController::stopController()
{
    if (!running)
        return;
    log<RestController>("Stopping controller...");
    running = false;
    listener.join();
    close(socketFd);
}