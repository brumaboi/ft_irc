#include "../includes/client.hpp"

// ----------------- Constructor / Destructor -----------------
Client::Client(int fd, const std::string &hostname) 
    : _fd(fd), _hostname(hostname)
{
    _initializeClient();
}

Client::~Client()
{
}

// ----------------- Private Helper Methods -----------------
void Client::_initializeClient()
{
    _nickname = "";
    _username = "";
    _realname = "";
    _registered = false;
    _passwordAccepted = false;
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

std::string Client::getUsername() const
{
    return _username;
}

void Client::setUsername(const std::string &username)
{
    _username = username;
}

std::string Client::getRealname() const
{
    return _realname;
}

void Client::setRealname(const std::string &realname)
{
    _realname = realname;
}

std::string Client::getHostname() const
{
    return _hostname;
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

bool Client::isPasswordAccepted() const
{
    return _passwordAccepted;
}

void Client::setPasswordAccepted(bool accepted)
{
    _passwordAccepted = accepted;
}
