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

    // Channel modes (basic set) !!!!!
    bool isOp(Client *client) const; // Check if client is an operator
    void addOp(Client *client);      // Add operator status to client
    void removeOp(Client *client);   // Remove operator status from client
    bool isInviteOnly() const;        // Check if channel is invite-only (+i)
    void setInviteOnly(bool inviteOnly); // Set invite-only mode (+i)
    bool hasKey() const;            // Check if channel has a key (+k)
    const std::string& getKey() const; // Get channel key (+k)
    void setKey(const std::string &key); // Set channel key (+k)
    void removeKey();               // Remove channel key (+k)
    size_t getUserLimit() const;     // Get user limit (+l)
    void setUserLimit(size_t limit); // Set user limit (+l)
    void removeUserLimit();          // Remove user limit (+l)
    bool isInvited(const std::string &nick) const; // Check if a nickname is invited
    void inviteNick(const std::string &nick); // Invite a nickname to the channel
    void uninviteNick(const std::string &nick); // Remove a nickname from the invite list
    
    // Basic utility
    bool isEmpty() const;

private:
    // Essential channel data
    std::string _name;              // Channel name (starts with # or &)
    std::string _topic;             // Channel topic
    time_t _creationTime;           // When channel was created
    std::vector<Client*> _clients;  // All channel members
    std::unordered_set<Client*> _ops; // Channel operators

    // Channel modes (basic set) !!!!!
    bool _inviteOnly = false;           // Invite-only mode (+i)
    std::string _key;               // Channel key for entry (+k)
    size_t _userLimit = 0;          // User limit (+l)
    std::unordered_set<std::string> _invitedNicks; // Invited nicknames for invite-only channels
    // More modes can be added as needed

    // Helper method
    void _initializeChannel();
};

