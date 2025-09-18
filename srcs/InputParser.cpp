#include "InputParser.hpp"
#include "server.hpp"
#include "channel.hpp"
#include "client.hpp"

InputParser::InputParser(Server& server) : server(server)
{
    registerHandlers();
}

static void UpperCommand(std::string& s)
{
    for (size_t i = 0; i < s.size(); ++i)
        s[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[i])));
}

void InputParser::onClientDisconnect(int fd)
{
    clientBuffers.erase(fd);
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
            tmp.clear();
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
        handleUnknownCommand(client, parsedInput);
    }
}

// Fixed processInput to handle \n endings, allowing JOIN and PRIVMSG commands to be tested correctly.
void InputParser::processInput(int fd, const std::string& bytes)
{
    // Append incoming bytes to the buffer for this client
    clientBuffers[fd] += bytes;
    std::string& buffer = clientBuffers[fd];

    std::size_t pos;
    while ((pos = buffer.find('\n')) != std::string::npos) // search for '\n' instead of only "\r\n"
    {
        std::string line = buffer.substr(0, pos);

        // Remove trailing '\r' if it exists
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        // Erase processed line from buffer
        buffer.erase(0, pos + 1);

        // Skip overly long lines
        if (line.size() > 512)
            continue;

        // Skip empty lines
        if (line.empty())
            continue;

        // Parse the line
        ParsedInput parsedInput = parseLine(line);

        // Get client object
        Client* client = server.getClientByFd(fd);
        if (client)
            handleCommand(*client, parsedInput);
    }

    // Prevent buffer from growing too large
    if (buffer.size() > 4096)
        buffer.erase(0, buffer.size() - 4096);
}
// void InputParser::processInput(int fd, const std::string& bytes)
// {
//     clientBuffers[fd] += bytes;
//     std::string& buffer = clientBuffers[fd];

//     std::size_t pos;
//     while ((pos = buffer.find("\r\n")) != std::string::npos)
//     {
//         std::string line = buffer.substr(0, pos);
//         buffer.erase(0, pos + 2);
//         if (line.size() + 2 > 512)
//             continue ;
//         if (line.empty())
//             continue ;

//         ParsedInput parsedInput = parseLine(line);
//         Client* client = server.getClientByFd(fd);
//         if (client)
//             handleCommand(*client, parsedInput);
//     }
//     if (buffer.size() > 4096)
//         buffer.erase(0, buffer.size() - 4096);
// }

void InputParser::handleUnknownCommand(Client& client, const ParsedInput& parsedInput)
{
    (void)parsedInput;
    server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :Unknown command\r\n");
}

void InputParser::registerHandlers()
{
    commandHandlers["JOIN"] = &InputParser::handleJoin;
    commandHandlers["PART"] = &InputParser::handlePart;
    commandHandlers["PRIVMSG"] = &InputParser::handlePrivMsg;
    commandHandlers["NICK"] = &InputParser::handleNick;
    commandHandlers["USER"] = &InputParser::handleUser;
    commandHandlers["PASS"] = &InputParser::handlePass;
    commandHandlers["PING"] = &InputParser::handlePing;
    commandHandlers["PONG"] = &InputParser::handlePong;
    commandHandlers["QUIT"] = &InputParser::handleQuit;
    commandHandlers["TOPIC"] = &InputParser::handleTopic;
    commandHandlers["MODE"] = &InputParser::handleMode;
    commandHandlers["INVITE"] = &InputParser::handleInvite;
    commandHandlers["KICK"] = &InputParser::handleKick;
}

void InputParser::handleJoin(Client& client, const ParsedInput& parsedInput)
{
    //
    // Should gatekeep to registered users only
    //
    if (parsedInput.args.empty())
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :No channel name given\r\n");
        return ;
    }
    std::string channelName = parsedInput.args[0];
    if (channelName.size() < 2 || (channelName[0] != '#' && channelName[0] != '&'))
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :Erroneous channel name\r\n");
        return ; 
    }
    Channel* channel = server.getOrCreateChannel(channelName);
    if (!channel)
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :Failed to create or join channel\r\n");
        return ;
    }
    if(!channel->hasClient(&client))
        channel->addClient(&client);
    const std::string joinMsg = ":" + client.getNickname() + " JOIN " + channelName + "\r\n";
    server.sendResponse(client.getFd(), joinMsg);
    std::vector<Client*> clients = channel->getClients();
    for (size_t i = 0; i < clients.size(); ++i)
    {
        if (clients[i] && clients[i] != &client)
            server.sendResponse(clients[i]->getFd(), joinMsg);
    }
    if (!channel->getTopic().empty())
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " TOPIC " + channelName + " :" + channel->getTopic() + "\r\n");
    }
    else
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " NOTICE " + client.getNickname() + " :No topic is set\r\n");
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
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :No channel name given\r\n");
        return ;
    }
    std::string channelName = parsedInput.args[0];

    Channel* channel = server.findChannelByName(channelName);
    if (!channel)
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :No such channel\r\n");
        return ;
    }
    if (!channel->hasClient(&client))
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :You're not on that channel\r\n");
        return ;
    }
    std::string partMsg = ":" + client.getNickname() + " PART " + channelName + "\r\n";
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
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :No password given\r\n");
        return ;
    }
    if (client.isRegistered())
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :You are already registered\r\n");
        return ;
    }
    std::string password = parsedInput.args[0];
    if (password != server.getPassword())
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :Password incorrect\r\n");
        return ;
    }
    client.setPasswordAccepted(true);
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
    (void)client;
    // client.//function to update last pong time or status();
    //
    // !!!!!
    //  
}

void InputParser::handleQuit(Client& client, const ParsedInput& parsedInput)
{
    std::string quitMsg = parsedInput.args.empty() ? "Client Quit" : parsedInput.args[0];
    if (!quitMsg.empty() && quitMsg[0] == ':')
        quitMsg = quitMsg.substr(1);
    std::string fullQuitMsg = ":" + client.getNickname() + " QUIT :" + quitMsg + "\r\n";
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

    if (client.isRegistered())
    {
        server.sendResponse(fd, ":" + server.getServerName() + " " + client.getNickname() + " :You are already registered\r\n");
        return ;
    }
    //need 4 params: <user> <mode> <unused> :<realname>
    if (parsedInput.args.size() < 4)
    {
        server.sendResponse(fd, ":" + server.getServerName() + " " + client.getNickname() + " :Not enough parameters\r\n");
        return ;
    }
    std::string username = parsedInput.args[0];
    std::string realname = parsedInput.args[3];
    if (!realname.empty() && realname[0] == ':')
        realname = realname.erase(0, 1);
    client.setUsername(username);
    client.setRealname(realname);

    const bool passRequired = !server.getPassword().empty();
    const bool passProvided = !passRequired || client.isPasswordAccepted();
    if (passProvided && !client.getNickname().empty())
    {
        client.setRegistered(true);
        server.sendResponse(fd, ":" + server.getServerName() + " 001 " + client.getNickname() + " :Welcome to the IRC Network\r\n");
        server.sendResponse(fd, ":" + server.getServerName() + " 002 " + client.getNickname() + " :Your host is " + server.getServerName() + "\r\n");
        server.sendResponse(fd, ":" + server.getServerName() + " 003 " + client.getNickname() + " :This server was created <date>\r\n");
        server.sendResponse(fd, ":" + server.getServerName() + " 004 " + client.getNickname() + " " + server.getServerName() + " <version> <available user modes> <available channel modes>\r\n");
    }
    else if (!passProvided)
    {
        server.sendResponse(fd, ":" + server.getServerName() + " " + client.getNickname() + " :Password required\r\n");
    }
    else if (client.getNickname().empty())
    {
        server.sendResponse(fd, ":" + server.getServerName() + " " + client.getNickname() + " :Nickname required\r\n");
    }
    
}

void InputParser::handlePrivMsg(Client& client, const ParsedInput& parsedInput)
{
    const int fd = client.getFd();

    if (parsedInput.args.size() < 2)
    {
        server.sendResponse(fd, ":" + server.getServerName() + " " + client.getNickname() + " :Not enough parameters\r\n");
        return ;
    }
    std::string target = parsedInput.args[0];
    std::string message = parsedInput.args[1];
    if (message.empty())
    {
        server.sendResponse(fd, ":" + server.getServerName() + " " + client.getNickname() + " :No text to send\r\n");
        return ;
    }
    if (!target.empty() && target[0] == '#')
    {
        Channel* channel = server.findChannelByName(target);
        if (!channel)
        {
            server.sendResponse(fd, ":" + server.getServerName() + " " + client.getNickname() + " :No such channel\r\n");
            return ;
        }
        if (!channel->hasClient(&client))
        {
            server.sendResponse(fd, ":" + server.getServerName() + " " + client.getNickname() + " :Cannot send to channel\r\n");
            return ;
        }
        const std::vector<Client*> clients = channel->getClients();
        for (size_t i = 0; i < clients.size(); ++i)
        {
           Client* targetClient = clients[i];
           if (targetClient && targetClient->getFd() != fd)
           {
               server.sendResponse(targetClient->getFd(), ":" + client.getNickname() + " PRIVMSG " + target + " :" + message + "\r\n");
           }
        }
    }
    else
    {
        Client* directClient = server.getClientByNick(target);
        if (!directClient)
        {
            server.sendResponse(fd, ":" + server.getServerName() + " " + client.getNickname() + " :No such nick/channel\r\n");
            return ;
        }
        server.sendResponse(directClient->getFd(), ":" + client.getNickname() + " PRIVMSG " + target + " :" + message + "\r\n");
    }
}

void InputParser::handleNick(Client& client, const ParsedInput& parsedInput)
{
    const int fd = client.getFd();
    const std::string& hostShown = server.getServerName();

    if (parsedInput.args.empty())
    {
        server.sendResponse(fd, ":" + server.getServerName() + " " + client.getNickname() + " :No nickname given\r\n");
        return ;
    }
    std::string newNick = parsedInput.args[0];
    static const std::regex nickRe("^[A-Za-z][A-Za-z0-9\\-_]{0,15}$"); //will need to change this logic
    if (!std::regex_match(newNick, nickRe))
    {
        server.sendResponse(fd, ":" + server.getServerName() + " " + client.getNickname() + " :Erroneous nickname\r\n");
        return ;
    }

    if (Client* existingClient = server.getClientByNick(newNick))
        if (existingClient && existingClient->getFd() != fd)
    {
        server.sendResponse(fd, ":" + server.getServerName() + " " + client.getNickname() + " :Nickname is already in use\r\n");
        return ;
    }

    std::string oldNick = client.getNickname();
    const bool wasRegistered = client.isRegistered();
    const std::string user = client.getUsername().empty() ? "*" : client.getUsername();
    
    std::string nickChangeMsg;
    if (wasRegistered && !oldNick.empty())
        nickChangeMsg = ":" + oldNick + "!" + user + "@" + hostShown + " NICK :" + newNick + "\r\n";
    else
        nickChangeMsg = ":" + hostShown + " NICK :" + newNick + "\r\n";
    client.setNickname(newNick);
}

void InputParser::handleInvite(Client& client, const ParsedInput& parsedInput)
{
    if (parsedInput.args.size() < 2)
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :Not enough parameters\r\n");
        return ;
    }
    const std::string targetNick = parsedInput.args[0];
    const std::string channelName = parsedInput.args[1];
    Channel* chan = server.findChannelByName(channelName);
    if (!chan)
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :No such channel\r\n");
        return ;
    }
    if (!chan->hasClient(&client))
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :You're not on that channel\r\n");
        return ;
    }
    if (chan->isInviteOnly() && !chan->isOp(&client))
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :You're not channel operator\r\n");
        return ;
    }

    Client* targetClient = server.getClientByNick(targetNick);
    if (!targetClient)
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :No such nick/channel\r\n");
        return ;
    }
    chan->inviteNick(targetNick);
    server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :You have invited " + targetNick + " to " + channelName + "\r\n");
    server.sendResponse(targetClient->getFd(), ":" + client.getNickname() + " INVITE " + targetNick + " :" + channelName + "\r\n");
}

void InputParser::handleKick(Client& client, const ParsedInput& parsedInput)
{
    (void)client;
    (void)parsedInput;
    // To be implemented
}

void InputParser::handleTopic(Client& client, const ParsedInput& parsedInput)
{
    if (parsedInput.args.empty())
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :No channel name given\r\n");
        return ;
    }
    const std::string channelName = parsedInput.args[0];
    Channel* chan = server.findChannelByName(channelName);
    if (!chan)
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :No such channel\r\n");
        return ;
    }
    if (!chan->hasClient(&client))
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :You're not on that channel\r\n");
        return ;
    }
    if (parsedInput.args.size() == 1)
    {
        if (chan->getTopic().empty())
        {
            server.sendResponse(client.getFd(), ":" + server.getServerName() + " NOTICE " + client.getNickname() + " :No topic is set\r\n");
        }
        else
        {
            server.sendResponse(client.getFd(), ":" + server.getServerName() + " TOPIC " + channelName + " :" + chan->getTopic() + "\r\n");
        }
        return ;
    }
    if (chan->topicOp() && !chan->isOp(&client))
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :You're not channel operator\r\n");
        return ;
    }
    chan->setTopic(parsedInput.args[1]);
    server.broadcastToChannel(&client, channelName, ":" + client.getNickname() + " TOPIC " + channelName + " :" + chan->getTopic() + "\r\n");
}

void InputParser::handleMode(Client& client, const ParsedInput& parsedInput)
{
    if (parsedInput.args.empty())
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :Not enough parameters\r\n");
        return ;
    }

    const std::string channel = parsedInput.args[0];
    if (channel.empty() || (channel[0] != '#' && channel[0] != '&'))
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :Erroneous channel name\r\n");
        return ;
    }

    Channel* chan = server.findChannelByName(channel);
    if (!chan)
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :No such channel\r\n");
        return ;
    }

    if (parsedInput.args.size() == 1)
    {
        std::string m = "+";
        std::string modes;
        if (chan->isInviteOnly())
            m += "i";
        if (chan->topicOp())
            m += "t";
        if (chan->hasKey())
        {
            m += "k";
            modes += " " + chan->getKey();
        }
        if (chan->getUserLimit())
        {
            m += "l";
            modes += " " + std::to_string(chan->getUserLimit());
        }
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " MODE " + channel + " " + m + modes + "\r\n");
        return ;
    }
    if (!chan->isOp(&client))
    {
        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :You're not channel operator\r\n");
        return ;
    }

    std::string flags = parsedInput.args[1];
    bool adding = true;
    size_t argIndex = 2;
    for (char f : flags)
    {
        if (f == '+')
        {
            adding = true;
            continue ;
        }
        if (f == '-')
        {
            adding = false;
            continue ;
        }
        switch (f)
        {
            case 'i':
                chan->setInviteOnly(adding);
                break ;
            case 't':
                chan->setTopicByOp(adding);
                break ;
            case 'k':
           {
                if (adding)
                {
                    if (chan->hasKey())
                    {
                        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :Channel already has a key\r\n");
                        return ;
                    }
                    if (argIndex >= parsedInput.args.size())
                    {
                        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :Key required\r\n");
                        return ;
                    }
                    std::string key = parsedInput.args[argIndex++];
                    chan->setKey(key);
                }
                else
                {
                    chan->removeKey();
                }
                break ;
            }
            case 'l':
            {
                if (adding)
                {
                    if (argIndex >= parsedInput.args.size())
                    {
                        server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :User limit required\r\n");
                        return ;
                    }
                    int limit = std::atoi(parsedInput.args[argIndex++].c_str());
                    if (limit <= 0)
                    {
                        chan->removeUserLimit();
                        break ;
                    }
                    chan->setUserLimit(limit);
                }
                else
                {
                    chan->removeUserLimit();
                }
                break ;
            }
            case 'o':
            {
                if (argIndex >= parsedInput.args.size())
                {
                    server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :User nickname required\r\n");
                    return ;
                }
                const std::string opNick = parsedInput.args[argIndex++];
                Client* opClient = server.getClientByNick(opNick);
                if (!opClient || !chan->hasClient(opClient))
                {
                    server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :No such nick/channel\r\n");
                    return ;
                }
                if (adding)
                    chan->addOp(opClient);
                else
                    chan->removeOp(opClient);
                break ;
            }
            default:
                server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :Unknown mode flag\r\n");
                return ;
        }
    }
    server.broadcastToChannel(&client, channel, ":" + client.getNickname() + " MODE " + channel + " :" + flags + "\r\n");
}
