#include <RestController.hpp>
#include <Database.hpp>
#include <Json.hpp>
#include <RequestHandler.hpp>

int main()
{
    RestController controller;
    registerHandlers(controller);
    controller.startController();
    char c;
    while ((c = getchar()) != 'x');
    return 0;
}