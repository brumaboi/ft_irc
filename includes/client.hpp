#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <algorithm>

class Channel;

class Client
{
public:
    // Constructor / Destructor
    Client(int fd, const std::string &hostname);
    ~Client();

    // Basic getters/setters
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
    void joinChannel(const std::string &channelName);
    void partChannel(const std::string &channelName);
    bool isMemberOf(const std::string &channelName) const;
    std::vector<std::string> getJoinedChannels() const;
    
    // Operator status
    bool isOperatorIn(const std::string &channelName) const;
    void setOperatorIn(const std::string &channelName, bool isOp);
    
    // Activity tracking
    time_t getLastActivity() const;
    void updateActivity();
    
    // Away status
    bool isAway() const;
    std::string getAwayMessage() const;
    void setAway(const std::string &message);
    void setBack();
    
    // Validation utilities - static methods (belong to class, not specific object)
    static bool isValidNickname(const std::string &nick);
    static bool isValidUsername(const std::string &username);

private:
    int _fd;                        // File descriptor for client socket
    std::string _hostname;
    std::string _nickname;          
    std::string _username;          
    std::string _realname;          
    bool _registered;          
    bool _passwordAccepted;

    // IRC registration sequence flags
    bool _receivedPass;             // Has received PASS command
    bool _receivedNick;
    bool _receivedUser;
    
    // Channel membership
    std::vector<std::string> _channels;  // List of joined channels
    std::map<std::string, bool> _operatorStatus;  // channelName -> isOperator
    
    // Activity tracking
    time_t _lastActivity;
    // Away status
    bool _isAway;
    std::string _awayMessage;
    
    void _initializeClient();
};
