#include "../includes/channel.hpp"
#include "../includes/client.hpp"
#include "../includes/logger.hpp"
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
    _topicOp = false;
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
    client->joinChannel(_name);
    Logger::info("Channel " + _name + ": Client " + client->getNickname() + " added (client now in " + 
                 std::to_string(client->getJoinedChannels().size()) + " channels)");
}

void Channel::removeClient(Client *client)
{
    if (!client)
        return;
    
    // Remove from client list
    std::vector<Client*>::iterator it = std::find(_clients.begin(), _clients.end(), client);
    if (it != _clients.end())
    {
        _clients.erase(it);
        client->partChannel(_name);
        Logger::info("Channel " + _name + ": Client " + client->getNickname() + " removed (client now in " + 
                     std::to_string(client->getJoinedChannels().size()) + " channels)");
    }
    
    // Also remove operator status
    removeOp(client);
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
    if (!client)
        return;
    _ops.insert(client);
    client->setOperatorIn(_name, true);
    Logger::info("Channel " + _name + ": Client " + client->getNickname() + 
                 " promoted to operator (client is now op in " + 
                 std::to_string(client->getJoinedChannels().size()) + " channels)");
}

void Channel::removeOp(Client *client)
{
    if (!client)
        return;
    _ops.erase(client);
    client->setOperatorIn(_name, false);
    Logger::info("Channel " + _name + ": Client " + client->getNickname() + " operator status removed");
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

bool Channel::topicOp() const
{
    return _topicOp;
}

void Channel::setTopicByOp(bool isOp)
{
    _topicOp = isOp;
}

// Checks if a client can join the channel (invite-only, key, user limit)
bool Channel::canJoin(Client* client, const std::string &key) const
{
    if (!client)
        return false;
    if (isInviteOnly() && !isInvited(client->getNickname()))
        return false;
    if (hasKey() && key != getKey())
        return false;
    if (getUserLimit() > 0 && getClientCount() >= getUserLimit())
        return false;
    return true;
}
