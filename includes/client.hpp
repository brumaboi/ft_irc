#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <ctime>

class Channel; // forward declaration

/* 
 * IRC Client Class
 * Represents a connected client with their state, permissions, and data
 */
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
    
    // Registration status
    bool isRegistered() const;
    void setRegistered(bool registered);

private:
    // Core client data - start with these essentials
    int _fd;                        // File descriptor for client socket
    std::string _hostname;          // Client hostname/IP
    std::string _nickname;          // IRC nickname
    bool _registered;               // Has completed registration
    
    // Helper method
    void _initializeClient();       // Initialize default values
};
