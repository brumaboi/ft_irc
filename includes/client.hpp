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

private:
    int _fd;                        // File descriptor for client socket
    std::string _hostname;          // Client hostname/IP
    std::string _nickname;          // IRC nickname
    std::string _username;          // IRC username (from user command)
    std::string _realname;          // IRC real name (from user command)
    bool _registered;               // Has completed registration
    bool _passwordAccepted;         // Password verification status
    
    void _initializeClient();       // Initialize default values
};
