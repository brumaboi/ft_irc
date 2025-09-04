#include "InputParser.hpp"

InputParser::InputParser(Server& server) : server(server)
{
    registerHandlers();
}

static void UpperCommand(std::string& s)
{
    for (size_t i = 0; i < s.size(); ++i)
        s[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[i])));
}

ParsedInput InputParser::parseLine(const std::string& line)
{

    ParsedInput parsed;
    std::string tmp = line;

    if (!tmp.empty() && tmp[0] == ':')
    {
        std::size_t spacePos = tmp.find(' ');
        if (spacePos != std::string::npos)
        {
            parsed.prefix = tmp.substr(1, spacePos - 1);
            tmp.erase(0, spacePos + 1);
        }
        else
        {
            tmp.clear(); // Malformed line, clear it
        }
    }

    std::string trailing;
    std::size_t colonPos = tmp.find(" :");
    if (colonPos != std::string::npos)
    {
        trailing = tmp.substr(colonPos + 2);
        tmp.erase(colonPos);
    }

    while (!tmp.empty() && tmp[0] == ' ')
        tmp.erase(0, 1);

    std::istringstream iss(tmp);
    iss >> parsed.command;
    UpperCommand(parsed.command);

    std::string arg;
    while (iss >> arg)
        parsed.args.push_back(arg);
    
    if (!trailing.empty())
        parsed.args.push_back(trailing);
    
    return parsed;
}

void InputParser::handleCommand(Client& client, const ParsedInput& parsedInput)
{
    std::unordered_map<std::string, CommandHandler>::iterator it = commandHandlers.find(parsedInput.command);
    if (it != commandHandlers.end())
    {
        CommandHandler handler = it->second;
        (this->*handler)(client, parsedInput);
    }
    else
    {
        // handleUnknownCommand(client, parsedInput);
    }
}

void InputParser::processInput(int fd, const std::string& bytes)
{
    clientBuffers[fd] += bytes;
    std::string& buffer = clientBuffers[fd];
    std::size_t pos;

    while ((pos = buffer.find("\r\n")) != std::string::npos)
    {
        std::string line = buffer.substr(0, pos);
        buffer.erase(0, pos + 2);
        ParsedInput parsedInput = parseLine(line);
        // Client* client = server.getClientByFd(fd);
        // if (client)
            // handleCommand(*client, parsedInput);
    }
}

void InputParser::registerHandlers()
{
    commandHandlers["JOIN"] = &InputParser::handleJoin;
    commandHandlers["PART"] = &InputParser::handlePart;
    // commandHandlers["PRIVMSG"] = &InputParser::handlePrivMsg;
    // commandHandlers["NICK"] = &InputParser::handleNick;
    // commandHandlers["USER"] = &InputParser::handleUser;
    // commandHandlers["PASS"] = &InputParser::handlePass;
    // commandHandlers["PING"] = &InputParser::handlePing;
    // commandHandlers["PONG"] = &InputParser::handlePong;
    // commandHandlers["QUIT"] = &InputParser::handleQuit;
    // commandHandlers["TOPIC"] = &InputParser::handleTopic;
    // commandHandlers["MODE"] = &InputParser::handleMode;
    // commandHandlers["INVITE"] = &InputParser::handleInvite;
    // commandHandlers["KICK"] = &InputParser::handleKick;
}

void InputParser::handleJoin(Client& client, const ParsedInput& parsedInput)
{
    if (parsedInput.args.empty())
        return; //send error for missing channel name
    std::string channelName = parsedInput.args[0];

    Channel* channel = server.//function that gets or creates a channel by name(channelName);
    if (!channel)
        return; //send error for channel creation failure
    
    if(!channel->hasClient(&client))
        channel->addClient(&client);
    
    //send confirmation to client
    const std::string joinMsg = ":" + client.getNick() + " JOIN " + channelName + "\r\n";
    server.sendResponse(client.getFd(), joinMsg);
    std::vector<Client*> clients = channel->getClients();
    for (size_t i = 0; i < clients.size(); ++i)
    {
        if (clients[i] != &client)
            server.sendResponse(clients[i]->getFd(), joinMsg);
    }

    // Optionally send topic and user list
    if (!channel->getTopic().empty())
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " TOPIC " + channelName + " :" + channel->getTopic() + "\r\n");
    }
    else
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " NOTICE " + client.getNick() + " :No topic is set\r\n");
    }
}

void InputParser::handlePart(Client& client, const ParsedInput& parsedInput)
{
    if (parsedInput.args.empty())
        return; //send error for missing channel name
    std::string channelName = parsedInput.args[0];

    Channel* channel = server.//function that gets a channel by name(channelName);
    if (!channel || !channel->hasClient(&client))
        return; //send errors
    
    std::string partMsg = ":" + client.getNick() + " PART " + channelName + "\r\n";
    server.sendResponse(client.getFd(), partMsg);
    std::vector<Client*> clients = channel->getClients();
    for (size_t i = 0; i < clients.size(); ++i)
    {
        if (clients[i] != &client)
            server.sendResponse(clients[i]->getFd(), partMsg);
    }
    channel->removeClient(&client);
}