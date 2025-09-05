#include "../includes/server.hpp"
#include "../includes/client.hpp"
#include "../includes/channel.hpp"


bool Server::_signalReceived = false;

// ----------------- Constructor / Destructor -----------------
Server::Server(const int port, const std::string &password)
    : _port(port), _password(password), _serverFd(-1), _shutdownRequested(false), _parser(*this)
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
        std::cout << "\nSignal received, stopping server..." << std::endl;
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

    std::cout << "Server listening on port " << _port << std::endl;
}

void Server::closeAll()
{
    for (size_t i = 0; i < _fds.size(); ++i)
        close(_fds[i].fd);
    _fds.clear();
    std::cout << "Server shutdown. All connections closed." << std::endl;
}

// ----------------- Run Loop -----------------
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

    std::cout << "New client connected, fd=" << clientFd << " host=" << hostname << std::endl;

}

void Server::receiveData(int fd)
{
    char buffer[1024];
    ssize_t bytesRead;

    // Clear the buffer before receiving data
    std::memset(buffer, 0, sizeof(buffer));

    // Receive data from the client socket
    bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);

    // If the client disconnected or an error occurred
    if (bytesRead <= 0)
    {
        std::cout << "Client disconnected, fd=" << fd << std::endl;

        // Close the client socket
        close(fd);

        // Remove the client from the poll vector
        _fds.erase(std::remove_if(_fds.begin(), _fds.end(),
            [fd](const pollfd &p) { return p.fd == fd; }),
            _fds.end());

        // Remove the client from the clients map
        // If using pointers, delete _clients[fd] before erasing
        _clients.erase(fd);

        return;
    }
    // Convert the received bytes to a string
    std::string msg(buffer, bytesRead);
    // Parser commented out for testing – currently does not return messages to the client
    //_parser.processInput(fd, msg);
    sendResponse(fd, "Server received: " + msg + "\n");
}

void	Server::removeClient(int fd)
{
	close(fd);
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

// Currently broadcasts messages to a channel, but since JOIN and channel management
// are not implemented yet, this will not send messages to other clients.
void Server::broadcastToChannel(const std::string &channelName, const std::string &message)
{
    // Get the channel by name; create it if it doesn't exist
    Channel* chan = getOrCreateChannel(channelName);
    if (!chan) return; // If channel couldn't be retrieved, exit

    // Loop through all clients in the channel
    for (Client* client : chan->getClients())
    {
        // Send the message to each client
        sendResponse(client->getFd(), message);
    }
}

void Server::addClientToChannel(const std::string &channelName, Client* client)
{
    if (!client)
        return;
    Channel* chan = getOrCreateChannel(channelName);
    if (chan)
    {
        // TODO: Broadcast a JOIN message to other clients in this channel
    //       Currently, other clients are not notified that a new client joined.
    //       In a real IRC server, you'd send something like:
    //       ":nickname!username@host JOIN #channel"
    }
}

void Server::removeClientFromChannel(Client* client)
{
    if (!client)
        return; // Avoid removing a null client

    for (auto it = _channels.begin(); it != _channels.end(); /* no increment here */) {
        Channel* chan = it->second;
        chan->removeClient(client); // Remove the client from this channel

        if (chan->getClients().empty()) {
            // Remove the channel if it has no clients
            delete chan; // Free memory
            it = _channels.erase(it); // erase returns the next iterator
        } else {
            // TODO: Broadcast a QUIT/PART message to other clients
            // Example: ":nickname!username@host PART #channel"
            ++it;
        }
    }
}

void Server::sendResponse(int fd, const std::string &message)
{
    send(fd, message.c_str(), message.size(), 0);
}
// ----------------- Command parsing placeholder -----------------
void	Server::parseCommand(int fd, const std::string &command)
{
	std::cout << "Received from fd " << fd << ": " << command << std::endl;
    sendResponse(fd, "Server got your message: " + command + "\n");
}

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

bool	Server::isNickInUse(const std::string &nick) const
{
    (void)nick;
	return false;
}