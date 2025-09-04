#include "../includes/server.hpp"

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

// void	Server::setupSocket(void)
// {
//     //Create TCP/IPv4 socket
// 	_serverFd = socket(AF_INET, SOCK_STREAM, 0);
// 	if (_serverFd < 0)
// 		throw std::runtime_error("socket() failed");

//     //Allow address reuse
// 	int	opt = 1;
// 	if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
// 		throw std::runtime_error("setsockopt() failed");

//     //Bind to port
// 	std::memset(&_address, 0, sizeof(_address));
// 	_address.sin_family = AF_INET;
// 	_address.sin_port = htons(_port);
// 	_address.sin_addr.s_addr = INADDR_ANY;

// 	if (bind(_serverFd, (struct sockaddr *)&_address,
// 			sizeof(_address)) < 0)
// 		throw std::runtime_error("bind() failed");

// 	if (listen(_serverFd, SOMAXCONN) < 0)
// 		throw std::runtime_error("listen() failed");

//     // Add server socket to poll list
// 	pollfd	pfd;
// 	pfd.fd = _serverFd;
// 	pfd.events = POLLIN;
// 	pfd.revents = 0;
// 	_fds.push_back(pfd);

// 	std::cout << "Server listening on port " << _port << std::endl;
// }

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
    // Currently only accepts, does not store clients
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
        return;
    }
    pollfd pfd;
    pfd.fd = clientFd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _fds.push_back(pfd);

    std::cout << "New client connected, fd=" << clientFd << std::endl;

}

void	Server::receiveData(int fd)
{
	char	buffer[1024];
	ssize_t	bytesRead;

	std::memset(buffer, 0, sizeof(buffer));
	bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);
	if (bytesRead <= 0)
	{
		std::cout << "Client disconnected, fd=" << fd << std::endl;
		close(fd);
		return;
	}
    _parser.processInput(fd, std::string(buffer, bytesRead));
}

void	Server::removeClient(int fd)
{
	close(fd);
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