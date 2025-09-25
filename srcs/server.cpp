#include "../includes/server.hpp"
#include "../includes/client.hpp"
#include "../includes/channel.hpp"
#include "../includes/irc_utils.hpp"
#include "../includes/logger.hpp"

bool Server::_signalReceived = false;

// ----------------- Constructor / Destructor -----------------
Server::Server(const int port, const std::string &password)
    : _port(port), _password(password), _serverFd(-1), _shutdownRequested(false), _closed(false), _parser(*this) 
{
    _serverName = "MyServer";
    setupSignalHandler();;
}

Server::~Server()
{
    closeAll();
}

// ----------------- Signal handling -----------------
void Server::signalHandler(int signum)
{
    if (signum == SIGINT)
    {
        Logger::info("Signal received, stopping server...");
        _signalReceived = true;
    }
}

void Server::setupSignalHandler()
{
    struct sigaction sa;
    sa.sa_handler = Server::signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, nullptr) == -1)
    {
        std::cout << "sigaction failed: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
}

void Server::setupSocket()
{
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd < 0)
        throw std::runtime_error("socket() failed");

    int opt = 1;
    if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");

    if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0)
        std::cerr << "Warning: setsockopt(SO_REUSEPORT) failed, continuing\n";

    std::memset(&_address, 0, sizeof(_address));
    _address.sin_family = AF_INET;
    _address.sin_addr.s_addr = INADDR_ANY;

    int attempts = 100;
    int port = _port;
    for (int i = 0; i < attempts; ++i)
    {
        _address.sin_port = htons(port);
        if (bind(_serverFd, (struct sockaddr *)&_address, sizeof(_address)) == 0)
        {
            _port = port;
            break;
        }
        else
        {
            std::cerr << "Port " << port << " in use, trying next port..." << std::endl;
            port++;
            if (i == attempts - 1)
                throw std::runtime_error(std::string("bind() failed on multiple ports: ") + strerror(errno));
        }
    }

    if (listen(_serverFd, SOMAXCONN) < 0)
        throw std::runtime_error("listen() failed");

    if (fcntl(_serverFd, F_SETFL, O_NONBLOCK) < 0)
        throw std::runtime_error("fcntl() failed");

    pollfd pfd;
    pfd.fd = _serverFd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _fds.push_back(pfd);

    // std::cout << "Server listening on port " << _port << std::endl;
    Logger::info("Server listening on port " + std::to_string(_port));
}

void Server::closeAll()
{
    if (_closed)
        return;
    _closed = true;

    for (size_t i = 0; i < _fds.size(); ++i)
        close(_fds[i].fd);
    _fds.clear();
    Logger::info("Server shutdown. All connections closed.");
}

//----------------- Run Loop -----------------
void Server::run()
{
    setupSocket();
    while (!_signalReceived)
    {
        int ret = poll(_fds.data(), _fds.size(), -1);
        if (ret < 0) continue;

        for (size_t i = 0; i < _fds.size(); i++)
        {
            if (_fds[i].revents & POLLIN)
            {
                if (_fds[i].fd == _serverFd)
                    acceptNewClient();
                else
                    receiveData(_fds[i].fd);
            }
        }
    }
    closeAll();
}


// ----------------- Client handling placeholders -----------------
void Server::acceptNewClient()
{
    sockaddr_in cliAddr;
    socklen_t len = sizeof(cliAddr);
    int clientFd = accept(_serverFd, (sockaddr *)&cliAddr, &len);
    if (clientFd < 0)
    {
        std::cerr << "accept() failed" << std::endl;
        return;
    }
    if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0)
    {
        std::cerr << "fcntl() failed" << std::endl;
        close(clientFd);
        return;
    }
    pollfd pfd;
    pfd.fd = clientFd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _fds.push_back(pfd);

    std::string hostname = inet_ntoa(cliAddr.sin_addr);
    Client* newClient = new Client(clientFd, hostname);
    _clients[clientFd] = newClient;

    // std::cout << "New client connected, fd=" << clientFd << " host=" << hostname << std::endl;
    Logger::log(LOG_CONNECTION, "New client connected, fd=" + std::to_string(clientFd) + " host=" + hostname);
    sendWelcomeInstructions(*this, clientFd);
}

void Server::receiveData(int fd)
{
    char buffer[1024];
    ssize_t bytesRead;

    std::memset(buffer, 0, sizeof(buffer));

    bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0)
    {
        // std::cout << "Client disconnected, fd=" << fd << std::endl;
        Logger::log(LOG_DISCONNECTION, "Client disconnected, fd=" + std::to_string(fd));
        removeClient(fd);
        return;
    }
    std::string data(buffer, bytesRead);
    _parser.processInput(fd, data);
}

void	Server::removeClient(int fd)
{
    _parser.onClientDisconnect(fd);

	close(fd);

    _fds.erase(std::remove_if(_fds.begin(), _fds.end(),
        [fd](const pollfd &p) { return p.fd == fd; }),
        _fds.end());
    
    auto it = _clients.find(fd);
    if (it != _clients.end())
    {
        delete it->second;
        _clients.erase(it);
    }
}

void Server::removeChannel(const std::string &name)
{
    auto it = _channels.find(name);
    if (it != _channels.end())
    {
        delete it->second;
        _channels.erase(it);
    }
}

// ----------------- Channel handling -----------------
Channel* Server::getOrCreateChannel(const std::string& name)
{
    Channel* chan;
    auto it = _channels.find(name);
    if (it != _channels.end())
        return it->second;

    chan = new Channel(name);
    _channels[name] = chan;
    return chan;
}

void Server::addClientToChannel(const std::string &channelName, Client* client, const std::string &providedKey)
{
	if (!client)
		return;

	Channel* chan = getOrCreateChannel(channelName);
	if (!chan)
		return;

	// Check if the client is allowed to join the channel
	if (!chan->canJoin(client, providedKey)) {
		sendResponse(client->getFd(),
			":" + getServerName() + " " + client->getNickname() + " :Cannot join channel\r\n");
		return;
	}

	// Add the client if not already in the channel
	if (!chan->hasClient(client))
		chan->addClient(client);

	// First client becomes operator
	if (chan->getClientCount() == 1)
		chan->addOp(client);

	// Prepare JOIN message
	std::string joinMsg = ":" + client->getNickname() + "!user@" + client->getHostname() + " JOIN " + channelName + "\r\n";

	// Send JOIN message to all clients in the channel including the joining client
	for (Client* c : chan->getClients()) {
		sendResponse(c->getFd(), joinMsg);
	}

	// If channel has a topic, send it to the joining client; otherwise, send a NOTICE
	if (!chan->getTopic().empty()) {
		sendResponse(client->getFd(),
			":" + getServerName() + " TOPIC " + channelName + " :" + chan->getTopic() + "\r\n");
	} else {
		sendResponse(client->getFd(),
			":" + getServerName() + " NOTICE " + client->getNickname() + " :No topic is set\r\n");
	}
}

void Server::broadcastToChannel(Client* sender, const std::string &channelName, const std::string &message)
{
	Channel* chan = findChannelByName(channelName);
	if (!chan)
		return;

	// Check if sender is part of the channel
	if (!chan->hasClient(sender)) {
		sendResponse(sender->getFd(),
			":" + getServerName() + " " + sender->getNickname() + " :Cannot send to channel\r\n");
		return;
	}

	// Send message to all clients **including the sender**
	for (Client* client : chan->getClients())
    {
        if (!sender || client->getFd() != sender->getFd())
		    sendResponse(client->getFd(), message);
	}
}

void Server::removeClientFromChannel(Client* client)
{
	if (!client)
		return;

	for (auto it = _channels.begin(); it != _channels.end(); /* no increment */) 
	{
		Channel* chan = it->second;
		if (chan->hasClient(client))
		{
			std::string partMsg = ":" + client->getNickname() + "!user@" + client->getHostname() + " PART " + chan->getName() + "\r\n";
			for (Client* c : chan->getClients())
			{
				if (c != client)
					sendResponse(c->getFd(), partMsg);
			}
			chan->removeClient(client);

			if (chan->getClients().empty())
			{
				delete chan;
				it = _channels.erase(it);
				continue;
			}
		}
		++it;
	}
}


Client* Server::getClientByFd(int fd) const
{
    std::map<int, Client*>::const_iterator it = _clients.find(fd);
    if (it != _clients.end())
        return it->second;
    return nullptr;
}


Channel* Server::findChannelByName(const std::string& name) const
{
    std::map<std::string, Channel*>::const_iterator it = _channels.find(name);
    if (it != _channels.end())
        return it->second;
    return nullptr;
}

std::vector<Channel*> Server::getChannelsForClient(Client* client) const
{
    std::vector<Channel*> result;
    for (std::map<std::string, Channel*>::const_iterator it = _channels.begin(); it != _channels.end(); ++it)
    {
        if (it->second->hasClient(client))
            result.push_back(it->second);
    }
    return result;
}

Client* Server::getClientByNick(const std::string &nick) const
{
    for (std::map<int, Client*>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (it->second->getNickname() == nick)
            return it->second;
    }
    return nullptr;
}

void Server::sendResponse(int fd, const std::string &message)
{
    send(fd, message.c_str(), message.size(), 0);
}

//--------------TEST--------------------------------------
// Handles and executes a single IRC command received from a client.
// This is a simplified parser used to test server functionality only.
// It supports core commands: NICK, USER, JOIN, PART, and PRIVMSG.
// Does not use the full InputParser implementation.
// Sends a default acknowledgment for unrecognized commands.
// Once all command handlers are complete, the server will delegate parsing to InputParser instead.
// void Server::parseCommand(int fd, const std::string &command)
// {
//     Client* client = getClientByFd(fd);
//     if (!client)
//         return;

//     // Remove trailing newline/carriage return
//     std::string cmd = command;
//     if (!cmd.empty() && cmd.back() == '\n') cmd.pop_back();
//     if (!cmd.empty() && cmd.back() == '\r') cmd.pop_back();

//     std::cout << "Received from fd " << fd << ": " << cmd << std::endl;

//     // ----- NICK command -----
//     if (cmd.rfind("NICK ", 0) == 0)
//     {
//         std::string nick = cmd.substr(5);
//         client->setNickname(nick);
//         std::cout << "DEBUG: Nickname set to " << nick << std::endl;
//         return;
//     }

//     // ----- USER command -----
//     if (cmd.rfind("USER ", 0) == 0)
//     {
//         // For simplicity, take the last part after ':' as username
//         size_t colonPos = cmd.find(':');
//         std::string username;
//         if (colonPos != std::string::npos)
//             username = cmd.substr(colonPos + 1);
//         else
//             username = cmd.substr(5);

//         client->setRegistered(true);
//         std::cout << "DEBUG: Username set to " << username << std::endl;
//         return;
//     }

//     // ----- JOIN command -----
//     if (cmd.rfind("JOIN ", 0) == 0)
//     {
//         std::string channelName = cmd.substr(5);
//         addClientToChannel(channelName, client);
//         return;
//     }

//     // ----- PART command -----
//     if (cmd.rfind("PART ", 0) == 0)
//     {
//         std::string channelName = cmd.substr(5);
//         Channel* chan = findChannelByName(channelName);
//         if (chan)
//         {
//             chan->removeClient(client);
//             removeClientFromChannel(client);
//         }
//         return;
//     }

//     // ----- PRIVMSG command -----
//     if (cmd.rfind("PRIVMSG ", 0) == 0)
//     {
//         size_t spacePos = cmd.find(' ', 8);
//         if (spacePos != std::string::npos)
//         {
//             std::string target = cmd.substr(8, spacePos - 8);
//             std::string message = cmd.substr(spacePos + 2);
//             broadcastToChannel(target, ":" + client->getNickname() +
//                 "!user@" + client->getHostname() +
//                 " PRIVMSG " + target + " :" + message + "\r\n");
//         }
//         return;
//     }

//     // ----- Default response -----
//     sendResponse(fd, "Server got your message: " + cmd + "\n");
// }

// ----------------- Getters -----------------
int			Server::getPort() const
{
	return _port;
}

std::string	Server::getPassword() const
{
	return _password;
}

std::string	Server::getServerName() const
{
	return _serverName;
}

// Checks if the given nickname is already in use among currently connected clients.
// Iterates through the _clients map and compares each client's nickname with the given one.
// Returns true if the nickname is taken, false if it is available.
bool	Server::isNickInUse(const std::string &nick) const
{
    std::map<int, Client*>::const_iterator it;
    for (it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (it->second->getNickname() == nick)
            return true;
    }
	return false;
}