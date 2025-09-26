#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <ctime>

class Channel;

class Client
{
public:
    // Constructor / Destructor
    Client(int fd, const std::string &hostname);
    ~Client();

    // Basic getters/setters - implement these first
    int getFd() const;
    std::string getNickname() const;
    void setNickname(const std::string &nick);
    std::string getUsername() const;
    void setUsername(const std::string &username);
    std::string getRealname() const;
    void setRealname(const std::string &realname);
    std::string getHostname() const;
    
    // Registration status
    bool isRegistered() const;
    void setRegistered(bool registered);
    bool isPasswordAccepted() const;
    void setPasswordAccepted(bool accepted);

    // IRC registration sequence tracking
    bool hasReceivedPass() const;
    void setReceivedPass(bool received);
    bool hasReceivedNick() const;
    void setReceivedNick(bool received);
    bool hasReceivedUser() const;
    void setReceivedUser(bool received);
    bool isFullyRegistered() const;
    
    // Channel membership
    void addChannel(const std::string &channelName);
    void removeChannel(const std::string &channelName);
    bool isInChannel(const std::string &channelName) const;
    std::vector<std::string> getChannels() const;
    
    // Validation utilities - static methods (belong to class, not specific object)
    static bool isValidNickname(const std::string &nick);
    static bool isValidUsername(const std::string &username);

private:
    int _fd;                        // File descriptor for client socket
    std::string _hostname;          // Client hostname/IP
    std::string _nickname;          // IRC nickname
    std::string _username;          // IRC username (from user command)
    std::string _realname;          // IRC real name (from user command)
    bool _registered;               // Has completed registration
    bool _passwordAccepted;         // Password verification status

    // IRC registration sequence flags
    bool _receivedPass;             // Has received PASS command
    bool _receivedNick;             // Has received NICK command  
    bool _receivedUser;             // Has received USER command
    
    // Channel membership
    std::vector<std::string> _channels;  // List of joined channels
    
    void _initializeClient();       // Initialize default values
};
