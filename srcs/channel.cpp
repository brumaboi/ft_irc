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
