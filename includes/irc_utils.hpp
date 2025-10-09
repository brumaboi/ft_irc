#pragma once
#include <string>

namespace Ui
{
    static const char* RED = "\033[31m";
    static const char* GREEN = "\033[32m";
    static const char* YELLOW = "\033[33m";
    static const char* BLUE = "\033[34m";
    static const char* RESET = "\033[0m";
}

inline std::string serverPrefix(const Server& server)
{
    return ":" + server.getServerName();
}

inline std::string userPrefix(const Client& client)
{
    const std::string user = client.getUsername().empty() ? "*" : client.getUsername();
    return ":" + client.getNickname() + "!" + user + "@" + client.getHostname();
}

inline void sendNotice(Server& server, int fd, const std::string& target, const std::string& message)
{
    server.sendResponse(fd, Ui::BLUE + serverPrefix(server) + target + " : " + Ui::RESET + message + "\r\n");
}

inline void sendError(Server& server, int fd, int code, const std::string& target, const std::string& message)
{
    server.sendResponse(fd, Ui::BLUE + serverPrefix(server) + " " + std::to_string(code) + " " + target + " : " + Ui::RED + message + Ui::RESET + "\r\n");
}

inline void sendWarning(Server& server, int fd, const std::string& target, const std::string& message)
{
    server.sendResponse(fd, Ui::BLUE + serverPrefix(server) + target + " : " + Ui::YELLOW + message + Ui::RESET + "\r\n");
}

inline void sendSuccess(Server& server, int fd, const std::string& target, const std::string& message)
{
    server.sendResponse(fd, Ui::BLUE + serverPrefix(server) + target + " : " + Ui::GREEN + message + Ui::RESET + "\r\n");
}

inline void sendWelcomeInstructions(Server& server, int fd)
{
    sendWarning(server, fd, "AUTH ", "Welcome to " + server.getServerName() + "!");
    sendNotice(server, fd, "AUTH ", "Please register using PASS, NICK, and USER commands to continue:");
    sendNotice(server, fd, "AUTH ", "Example of usage:");
    sendNotice(server, fd, "AUTH ", "PASS <password>");
    sendNotice(server, fd, "AUTH ", "NICK <your_nickname>");
    sendNotice(server, fd, "AUTH ", "USER <username> * * :<realname>");
}
