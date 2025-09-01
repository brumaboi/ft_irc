#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <ctime>

class Client; // forward declaration

/*
 * IRC Channel Class - Basic Implementation
 * Start with essential channel functionality
 */
class Channel
{
public:
    // Constructor / Destructor
    Channel(const std::string &name);
    ~Channel();

    // Basic channel info
    std::string getName() const;
    std::string getTopic() const;
    void setTopic(const std::string &topic);
    
    // Member management - start with these basics
    void addClient(Client *client);
    void removeClient(Client *client);
    bool hasClient(Client *client) const;
    std::vector<Client*> getClients() const;
    size_t getClientCount() const;
    
    // Basic utility
    bool isEmpty() const;

private:
    // Essential channel data
    std::string _name;              // Channel name (starts with # or &)
    std::string _topic;             // Channel topic
    time_t _creationTime;           // When channel was created
    std::vector<Client*> _clients;  // All channel members
    
    // Helper method
    void _initializeChannel();
};
