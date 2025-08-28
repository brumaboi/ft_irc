#include "../includes/logger.hpp"

void Logger::log(LogLevel level, const std::string &message, bool exitAfter)
{
	std::string prefix;
	std::string color;

	switch (level)
	{
		case LOG_INFO:         prefix = "[INFO] ";          color = "\033[32m"; break;
		case LOG_WARNING:      prefix = "[WARN] ";          color = "\033[33m"; break;
		case LOG_ERROR:        prefix = "[ERROR] ";         color = "\033[31m"; break; 
		case LOG_CONNECTION:   prefix = "[CONNECT] ";       color = "\033[36m"; break; 
		case LOG_DISCONNECTION:prefix = "[DISCONNECT] ";    color = "\033[35m"; break; 
		case LOG_PING:         prefix = "[PING] ";          color = "\033[34m"; break; 
		case LOG_PONG:         prefix = "[PONG] ";          color = "\033[36m"; break; 
		case LOG_CHANNEL:      prefix = "[CHANNEL] ";       color = "\033[33m"; break;
		case LOG_PRIVMSG:      prefix = "[PRIVMSG] ";       color = "\033[32m"; break; 
	}

	std::cout << color << prefix << message << "\033[0m" << std::endl;

	if (exitAfter && level == LOG_ERROR)
		exit(EXIT_FAILURE);
}

void Logger::info(const std::string &message)				{ log(LOG_INFO, message); }
void Logger::warning(const std::string &message)			{ log(LOG_WARNING, message); }
void Logger::error(const std::string &message, bool exitAfter)	{ log(LOG_ERROR, message, exitAfter); }

