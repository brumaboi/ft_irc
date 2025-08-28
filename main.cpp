#include "includes/server.hpp"
#include "includes/logger.hpp"

int	main(int argc, char **argv)
{
	if (argc != 3)
	{
		Logger::error("Usage: " + std::string(argv[0]) + " <port> <password>", true);
		return 1;
	}

	int port = std::atoi(argv[1]);
	std::string password = argv[2];

	try
	{
		Logger::info("Starting server on port " + std::to_string(port));
		Server server(port, password);
		server.run();
	}
	catch (const std::exception &e)
	{
		Logger::error("Server error: " + std::string(e.what()), true);
		return 1;
	}
	return 0;
}

// int	main(int argc, char **argv)
// {
// 	if (argc != 3)
// 	{
// 		std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
// 		return (1);
// 	}
// 	int		port = std::atoi(argv[1]);
// 	std::string	password = argv[2];

// 	try
// 	{
// 		Server	server(port, password);
// 		server.run();
// 	}
// 	catch (const std::exception &e)
// 	{
// 		std::cerr << "Server error: " << e.what() << std::endl;
// 		return (1);
// 	}
// 	return (0);


//ft_irc % ./ircserv 6667 mypassword      ft_irc % nc 127.0.0.1 6667
//Server listening on port 6667           hello
//New client connected, fd=4              Server got your message: hello
//Received from fd 4: hello

//ctrl + c