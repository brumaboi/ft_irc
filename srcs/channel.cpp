#include "../includes/channel.hpp"
#include "../includes/client.hpp"
#include <algorithm>

// ----------------- Constructor / Destructor -----------------
Channel::Channel(const std::string &name) : _name(name)
{
    _initializeChannel();
}

Channel::~Channel()
{
    // Clean up - remove all clients
    _clients.clear();
}

// ----------------- Private Helper Methods -----------------
void Channel::_initializeChannel()
{
    _creationTime = time(NULL);
    _topic = "";
}

// ----------------- Basic Channel Info -----------------
std::string Channel::getName() const
{
    return _name;
}

std::string Channel::getTopic() const
{
    return _topic;
}

void Channel::setTopic(const std::string &topic)
{
    _topic = topic;
}

// ----------------- Member Management -----------------
void Channel::addClient(Client *client)
{
    if (!client || hasClient(client))
        return;
    
    _clients.push_back(client);
}

void Channel::removeClient(Client *client)
{
    if (!client)
        return;
    
    // Remove from client list
    std::vector<Client*>::iterator it = std::find(_clients.begin(), _clients.end(), client);
    if (it != _clients.end())
        _clients.erase(it);
}

bool Channel::hasClient(Client *client) const
{
    if (!client)
        return false;
    return std::find(_clients.begin(), _clients.end(), client) != _clients.end();
}

std::vector<Client*> Channel::getClients() const
{
    return _clients;
}

size_t Channel::getClientCount() const
{
    return _clients.size();
}

// ----------------- Utility Methods -----------------
bool Channel::isEmpty() const
{
    return _clients.empty();
}


// ----------------- Channel Modes -----------------

bool Channel::isOp(Client *client) const
{
    if (!client)
        return false;
    return _ops.count(client);
}

void Channel::addOp(Client *client)
{
    _ops.insert(client);
}

void Channel::removeOp(Client *client)
{
    _ops.erase(client);
}

bool Channel::isInviteOnly() const
{
    return _inviteOnly;
}

void Channel::setInviteOnly(bool inviteOnly)
{
    _inviteOnly = inviteOnly;
}

bool Channel::hasKey() const
{
    return !_key.empty();
}

const std::string& Channel::getKey() const
{
    return _key;
}

void Channel::setKey(const std::string &key)
{
    _key = key;
}

void Channel::removeKey()
{
    _key.clear();
}

size_t Channel::getUserLimit() const
{
    return _userLimit;
}

void Channel::setUserLimit(size_t limit)
{
    _userLimit = limit;
}

void Channel::removeUserLimit()
{
    _userLimit = 0;
}

bool Channel::isInvited(const std::string &nick) const
{
    return _invitedNicks.count(nick);
}

void Channel::inviteNick(const std::string &nick)
{
    if (!nick.empty())
        _invitedNicks.insert(nick);
}

void Channel::uninviteNick(const std::string &nick)
{
    _invitedNicks.erase(nick);
}

