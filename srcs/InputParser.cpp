#include "InputParser.hpp"
#include <regex>
#include <unordered_set>

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
    if (!channel->getTopic().empty())
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " TOPIC " + channelName + " :" + channel->getTopic() + "\r\n");
    }
    else
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " NOTICE " + client.getNick() + " :No topic is set\r\n");
    }
    //
    // Need to implement restrictions key(+k), invite only(+i), user limit(+l)
    // First user to join becomes operator(+o)
    // Send appropriate messages for these conditions
    // 
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
    //
    // If channel is empty after part, we can delete it from server's channel list
    // If the parting client was an operator, assign a new operator if needed
    // Need to add the right error messages and confirmations
    //
}

void InputParser::handlePass(Client& client, const ParsedInput& parsedInput)
{
    if (parsedInput.args.empty())
        return; //send error for missing password
    std::string password = parsedInput.args[0];

    if (password != server.getPassword())
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNick() + " :Password incorrect\r\n");
        return ;
    }
    //
    // Need updated Client class to complete Pass handling
    // We need to check if the client is already registered
    // If not, mark as registered and send welcome messages
    // If already registered, possibly ignore or send a notice
    // 
    // Password mismatch handling or other logic as needed
    //
}

void InputParser::handlePing(Client& client, const ParsedInput& parsedInput)
{
    const std::string token = parsedInput.args.empty() ? "" : parsedInput.args[0];
    server.sendResponse(client.getFd(), ":" + server.getServerName() + " PONG " + server.getServerName() + " :" + token + "\r\n");
    //
    // May need to add some checking and logging here
    //
}

void InputParser::handlePong(Client& client, const ParsedInput& parsedInput)
{
    (void)parsedInput;
    client.//function to update last pong time or status();
    //
    // !!!!!
    //  
}

void InputParser::handleQuit(Client& client, const ParsedInput& parsedInput)
{
    std::string quitMsg = parsedInput.args.empty() ? "Client Quit" : parsedInput.args[0];
    if (!quitMsg.empty() && quitMsg[0] == ':')
        quitMsg = quitMsg.substr(1);
    std::string fullQuitMsg = ":" + client.getNick() + " QUIT :" + quitMsg + "\r\n";

    // Notify all channels the client is part of
    std::vector<Channel*> channels = server.getChannelsForClient(&client);
    for (size_t i = 0; i < channels.size(); ++i)
    {
        std::vector<Client*> clients = channels[i]->getClients();
        for (size_t j = 0; j < clients.size(); ++j)
        {
            if (clients[j] != &client)
                server.sendResponse(clients[j]->getFd(), fullQuitMsg);
        }
        channels[i]->removeClient(&client);
        //
        // If channel is empty after removal, delete it from server's channel list
        //
    }
    //
    // Need to implement server function to remove client by reference or fd
    // May also need to tweak the Quit message format and handling
    //                          
}

void InputParser::handleUser(Client& client, const ParsedInput& parsedInput)
{
    const int fd = client.getFd();

    //check if it's already registered
    if (client.isRegistered())
    {
        server.sendResponse(fd, ":" + server.getServerName() + " " + client.getNick() + " :You are already registered\r\n");
        return;
    }
    //need 4 params: <user> <mode> <unused> :<realname>
    if (parsedInput.args.size() < 4)
    {
        server.sendResponse(fd, ":" + server.getServerName() + " " + client.getNick() + " :Not enough parameters\r\n");
        return;
    }
    std::string username = parsedInput.args[0];
    std::string realname = parsedInput.args[3];
    if (!realname.empty() && realname[0] == ':')
        realname = realname.erase(0, 1);
    //
    // need functions in Client class to set username and realname    
    // client.setUsername(username);
    // client.setRealname(realname);
    //

    // complete registration of user if PASS (if required) and NICK are set
    const bool passRequired = !server.getPassword().empty();
    const bool passProvided = !passRequired || client.// function to check if PASS was provided();
    if (passProvided && !client.getNickname().empty())
    {
        client.setRegistered(true);
        // send welcome messages
        server.sendResponse(fd, ":" + server.getServerName() + " 001 " + client.getNick() + " :Welcome to the IRC Network\r\n");
        server.sendResponse(fd, ":" + server.getServerName() + " 002 " + client.getNick() + " :Your host is " + server.getServerName() + "\r\n");
        server.sendResponse(fd, ":" + server.getServerName() + " 003 " + client.getNick() + " :This server was created <date>\r\n");
        server.sendResponse(fd, ":" + server.getServerName() + " 004 " + client.getNick() + " " + server.getServerName() + " <version> <available user modes> <available channel modes>\r\n");
    }
    else if (!passProvided)
    {
        server.sendResponse(fd, ":" + server.getServerName() + " " + client.getNick() + " :Password required\r\n");
    }
    else if (client.getNickname().empty())
    {
        server.sendResponse(fd, ":" + server.getServerName() + " " + client.getNick() + " :Nickname required\r\n");
    }
}

void InputParser::handlePrivMsg(Client& client, const ParsedInput& parsedInput)
{
    const int fd = client.getFd();

    if (parsedInput.args.size() < 2)
    {
        server.sendResponse(fd, ":" + server.getServerName() + " " + client.getNick() + " :Not enough parameters\r\n");
        return ;
    }
    std::string target = parsedInput.args[0];
    std::string message;
    for (size_t i = 1; i < parsedInput.args.size(); ++i)
    {
        if (!message.empty())
            message += " ";
        message += parsedInput.args[i];
    }
    if (!message.empty() && message[0] == ':')
        message = message.erase(0, 1);
    if (message.empty())
    {
        server.sendResponse(fd, ":" + server.getServerName() + " " + client.getNick() + " :No text to send\r\n");
        return ;
    }

    if (!target.empty() && target[0] == '#')
    {
        Channel* channel = server.//function to get channel by name(target);
        if (!channel)
        {
            server.sendResponse(fd, ":" + server.getServerName() + " " + client.getNick() + " :No such channel\r\n");
            return ;
        }
        if (!channel->hasClient(&client))
        {
            server.sendResponse(fd, ":" + server.getServerName() + " " + client.getNick() + " :Cannot send to channel\r\n");
            return ;
        }
        const std::vector<Client*> clients = channel->getClients();
        for (size_t i = 0; i < clients.size(); ++i)
        {
           Client* targetClient = clients[i];
           if (targetClient && targetClient->getFd() != fd)
           {
               server.sendResponse(targetClient->getFd(), ":" + client.getNick() + " PRIVMSG " + target + " :" + message + "\r\n");
           }
        }
    }
    else
    {
        Client* directClient = server.//function to get client by nickname(target);
        if (!directClient)
        {
            server.sendResponse(fd, ":" + server.getServerName() + " " + client.getNick() + " :No such nick/channel\r\n");
            return ;
        }
        server.sendResponse(directClient->getFd(), ":" + client.getNick() + " PRIVMSG " + target + " :" + message + "\r\n");
    }
}

void InputParser::handleNick(Client& client, const ParsedInput& parsedInput)
{
    const int fd = client.getFd();
    const std::string& hostShown = server.getServerName();

    if (parsedInput.args.empty())
    {
        server.sendResponse(fd, ":" + server.getServerName() + " " + client.getNick() + " :No nickname given\r\n");
        return ;
    }

    std::string newNick = parsedInput.args[0];

    static const std::regex nickRe("^[A-Za-z][A-Za-z0-9\\-_]{0,15}$");
    if (!std::regex_match(newNick, nickRe))
    {
        server.sendResponse(fd, ":" + server.getServerName() + " " + client.getNick() + " :Erroneous nickname\r\n");
        return ;
    }

    if (Client* existingClient = server.something);//function to get client by nickname(newNick))
        if (existingClient && existingClient->getFd() != fd)
    {
        server.sendResponse(fd, ":" + server.getServerName() + " " + client.getNick() + " :Nickname is already in use\r\n");
        return ;
    }

    std::string oldNick = client.getNick();
    const bool wasRegistered = client.isRegistered();
    const std::string user = client.getUsername().empty() ? "*" : client.getUsername();
    
    std::string nickChangeMsg;
    if (wasRegistered && !oldNick.empty())
        nickChangeMsg = ":" + oldNick + "!" + user + "@" + hostShown + " NICK :" + newNick + "\r\n";
    else
        nickChangeMsg = ":" + hostShown + " NICK :" + newNick + "\r\n";
    
    client.setNick(newNick);
}