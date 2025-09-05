#include "includes/server.hpp"
#include "includes/logger.hpp"
#include "includes/channel.hpp"
#include "includes/client.hpp"

int main(int argc, char **argv)
{
    // Check if the correct number of arguments is provided
    if (argc != 3)
    {
        Logger::error("Usage: " + std::string(argv[0]) + " <port> <password>", true);
        return 1;
    }

    // Parse the port number and password
    int port = std::atoi(argv[1]);
    std::string password = argv[2];

    // Validate port number (must be between 1 and 65535)
    if (port <= 0 || port > 65535)
    {
        Logger::error("Invalid port number: " + std::to_string(port), true);
        return 1;
    }

    // Ignore SIGPIPE signals to prevent server crash when sending to a closed socket
    std::signal(SIGPIPE, SIG_IGN);

    try
    {
        // Log server start
        Logger::info("Starting server on port " + std::to_string(port));

        // Create server object
        Server server(port, password);

        // Run the server main loop (blocking call)
        server.run();
    }
    catch (const std::exception &e)
    {
        // Log any runtime errors
        Logger::error("Server error: " + std::string(e.what()), true);
        return 1;
    }

    // Log server exit (graceful shutdown)
    Logger::info("Server exited gracefully");
    return 0;
}
