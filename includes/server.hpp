#pragma once

#include <iostream>
#include <cerrno>
#include <cstring>
#include <string>
#include <vector>
#include <poll.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <csignal>
#include <signal.h>
#include <fcntl.h>
#include <map>
#include <InputParser.hpp>

class Client; // forward declaration
class Channel;
class CommandParser;
class InputParser;

class Server
{
public:
    // Constructor / Destructor
    Server(const int port, const std::string &password);
    ~Server();

    // Main server loop
    void run();

    // Request a safe server shutdown
    void requestShutdown();

    // Client handling
    void acceptNewClient();        // Accept a new incoming client
    void receiveData(int fd);      // Receive data from a specific client
    void removeClient(int fd);     // Remove a client from server

    // Send messages to clients
    void sendResponse(int fd, const std::string &message);

    // Getters
    int getPort() const;           // Get server port
    std::string getPassword() const; // Get server password
    std::string getServerName() const; // Get server name

private:
    int _port;                      // Port server is listening on
        std::string _password;           // Connection password
    int _serverFd;                   // Server socket file descriptor
    struct sockaddr_in  _address;    // Server address structure
    std::string _serverName;         // Server name
    bool _shutdownRequested;         // Flag for server shutdown

    // Network state
    std::vector<pollfd> _fds;       // Poll descriptors for server + clients
    std::map<int, Client*> _clients; // Map of fd -> Client objects
    
    // Command processing
    CommandParser *_commandParser;   // Command parser instance

    // Socket setup and cleanup
    void setupSocket();          // Prepare server socket
    void closeAll();             // Close server and client sockets

    // Command parsing
    void parseCommand(int fd, const std::string &command);

    // Signal handling
    static bool _signalReceived;             // Flag for signal received
    static void signalHandler(int signum);    // Handle SIGINT (Ctrl+C)
    void setupSignalHandler();

    // Client management methods - add these
    Client* getClientByFd(int fd) const;             // Get client by file descriptor
    Client* getClientByNick(const std::string &nick) const; // Get client by nickname
    
    // Utility methods
    bool isNickInUse(const std::string &nick) const; // Check if nickname is already taken

    InputParser _parser;
};
