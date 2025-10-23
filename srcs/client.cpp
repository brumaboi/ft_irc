#include "../includes/client.hpp"
// constructor-destructor
Client::Client(int fd, const std::string &hostname) 
    : _fd(fd), _hostname(hostname)
{
    _initializeClient();
}

Client::~Client()
{
}

//private helper method
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
    
    // Initialize activity tracking to current time
    _lastActivity = std::time(NULL);
    
    // Initialize away status
    _isAway = false;
    _awayMessage = "";
}

//basic getters/setters
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

//registration status
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

//validation methods
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

//irc registration sequence
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
// Client is fully registered when:
    // 1. Password is accepted (if server requires one)
    // 2. NICK command was received and nickname is set
    // 3. USER command was received and username is set

//channel membership
std::vector<std::string> Client::getJoinedChannels() const
{
    return _channels;
}

void Client::joinChannel(const std::string &channelName)
{
    // Only add if not already in the channel
    if (!isMemberOf(channelName))
        _channels.push_back(channelName);
}

void Client::partChannel(const std::string &channelName)
{
    // Remove channel from list
    _channels.erase(std::remove(_channels.begin(), _channels.end(), channelName), _channels.end());
}

bool Client::isMemberOf(const std::string &channelName) const
{
    // Check if client is in the specified channel
    return std::find(_channels.begin(), _channels.end(), channelName) != _channels.end();
}

// Operator status management
bool Client::isOperatorIn(const std::string &channelName) const
{
    // Search for the channel in the operator status map
    std::map<std::string, bool>::const_iterator it = _operatorStatus.find(channelName);
    
    // If found AND the value is true, client is an operator
    if (it != _operatorStatus.end() && it->second == true)
        return true;
    
    // Otherwise, not an operator (or not in channel)
    return false;
}

void Client::setOperatorIn(const std::string &channelName, bool isOp)
{
    // If setting to operator (true)
    if (isOp)
    {
        // Add or update the channel in the map with value = true
        _operatorStatus[channelName] = true;
    }
    else
    {
        // If removing operator status (false)
        // Find the channel in the map
        std::map<std::string, bool>::iterator it = _operatorStatus.find(channelName);
        
        // If found, remove it from the map entirely
        if (it != _operatorStatus.end())
            _operatorStatus.erase(it);
    }
}

// Activity tracking
time_t Client::getLastActivity() const
{
    return _lastActivity;
}

void Client::updateActivity()
{
    _lastActivity = std::time(NULL);
}

// Away status management
bool Client::isAway() const
{
    return _isAway;
}

std::string Client::getAwayMessage() const
{
    return _awayMessage;
}

void Client::setAway(const std::string &message)
{
    _isAway = true;
    _awayMessage = message;
}

void Client::setBack()
{
    _isAway = false;
    _awayMessage = "";
}
