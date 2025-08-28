#pragma once

#include <iostream>
#include <string>

enum LogLevel
{
    LOG_INFO, // General informational messages about server events
    LOG_WARNING, // Warnings about unusual situations that are not fatal
    LOG_ERROR,    // Errors that might require attention, can optionally exit
    LOG_CONNECTION,  
    LOG_DISCONNECTION,
    LOG_PING,
    LOG_PONG,
    LOG_CHANNEL,
    LOG_PRIVMSG,
};


class Logger
{
public:
    static void log(LogLevel level, const std::string& message, bool exitAfter = false);
    static void info(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message, bool exitAfter = false);
};
