#include "../includes/client.hpp"

// ----------------- Constructor / Destructor -----------------
Client::Client(int fd, const std::string &hostname) 
    : _fd(fd), _hostname(hostname)
{
    _initializeClient();
}

Client::~Client()
{
    // Basic cleanup - for now just close the connection
    // Later: notify channels, send QUIT message, etc.
}

// ----------------- Private Helper Methods -----------------
void Client::_initializeClient()
{
    _nickname = "";
    _registered = false;
}

// ----------------- Basic Getters/Setters -----------------
int Client::getFd() const
{
    return _fd;
}

std::string Client::getNickname() const
{
    return _nickname;
}

void Client::setNickname(const std::string &nick)
{
    _nickname = nick;
}

// ----------------- Registration Status -----------------
bool Client::isRegistered() const
{
    return _registered && !_nickname.empty();
}

void Client::setRegistered(bool registered)
{
    _registered = registered;
}
