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

    _receivedPass = false;
    _receivedNick = false;
    _receivedUser = false;
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
    // IRC registration requires: password (if set), nickname, and username
    return _registered && !_nickname.empty() && !_username.empty() && _passwordAccepted;
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

// ----------------- Validation Methods -----------------
bool Client::isValidNickname(const std::string &nick)
{
    // IRC nickname rules:
    // - Length: 1-9 characters (RFC 2812)
    // - First character: letter (A-Z, a-z)
    // - Other characters: letters, digits, or special chars: _ - [ ] { } \ |
    
    if (nick.empty() || nick.length() > 9)
        return false;
    
    // First character must be a letter
    if (!std::isalpha(nick[0]))
        return false;
    
    // Check remaining characters
    for (size_t i = 1; i < nick.length(); ++i)
    {
        char c = nick[i];
        if (!std::isalnum(c) && c != '_' && c != '-' && c != '[' && c != ']' && 
            c != '{' && c != '}' && c != '\\' && c != '|')
            return false;
    }
    
    return true;
}

bool Client::isValidUsername(const std::string &username)
{
    // IRC username rules:
    // - Length: 1-10 characters (common limitation)
    // - No spaces, @, !, :, #, & (IRC special characters)
    
    if (username.empty() || username.length() > 10)
        return false;
    
    // Check for forbidden characters
    for (char c : username)
    {
        if (std::isspace(c) || c == '@' || c == '!' || c == ':' || 
            c == '#' || c == '&')
            return false;
    }
    
    return true;
}

// ----------------- IRC Registration Sequence -----------------
bool Client::hasReceivedPass() const
{
    return _receivedPass;
}

void Client::setReceivedPass(bool received)
{
    _receivedPass = received;
}

bool Client::hasReceivedNick() const
{
    return _receivedNick;
}

void Client::setReceivedNick(bool received)
{
    _receivedNick = received;
}

bool Client::hasReceivedUser() const
{
    return _receivedUser;
}

void Client::setReceivedUser(bool received)
{
    _receivedUser = received;
}

bool Client::isFullyRegistered() const
{
    return _passwordAccepted && _receivedNick && _receivedUser && 
           !_nickname.empty() && !_username.empty();
}

// ----------------- Channel Membership -----------------
std::vector<std::string> Client::getChannels() const
{
    return _channels;
}
// Client is fully registered when:
    // 1. Password is accepted (if server requires one)
    // 2. NICK command was received and nickname is set
    // 3. USER command was received and username is set