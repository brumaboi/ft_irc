#include "InputParser.hpp"
#include "server.hpp"
#include "channel.hpp"
#include "client.hpp"
#include "logger.hpp"
#include "irc_utils.hpp"

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

bool InputParser::requireRegistration(Client& client, const std::string& command)
{
    (void)command; // currently unused
    if (!client.isRegistered())
    {
        sendError(server, client.getFd(), client.getNickname(), "You are not registered");
        Logger::warning("Unregistered client " + client.getNickname() + " attempted command " + command);
        return false;
    }
    return true;
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
    Logger::info("CMD " + parsedInput.command + " from " + client.getNickname());

    std::unordered_map<std::string, CommandHandler>::iterator it = commandHandlers.find(parsedInput.command);
    if (it != commandHandlers.end())
    {
        CommandHandler handler = it->second;
        (this->*handler)(client, parsedInput);
    }
    else
    {
        Logger::warning("Unknown command " + parsedInput.command + " from " + client.getNickname());
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

void InputParser::handleUnknownCommand(Client& client, const ParsedInput& parsedInput)
{
    (void)parsedInput;
    sendError(server, client.getFd(), client.getNickname(), "Unknown command " + parsedInput.command);
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
    if (!requireRegistration(client, "JOIN"))
        return ;
    if (parsedInput.args.empty())
    {
        sendError(server, client.getFd(), client.getNickname(), "No channel name given");
        return ;
    }
    std::string channelName = parsedInput.args[0];
    if (channelName.size() < 2 || (channelName[0] != '#' && channelName[0] != '&'))
    {
        sendError(server, client.getFd(), client.getNickname(), "Erroneous channel name");
        return ; 
    }
    Channel* channel = server.getOrCreateChannel(channelName);
    if (!channel)
    {
        sendError(server, client.getFd(), client.getNickname(), "Failed to create or find channel");
        return ;
    }
    if(!channel->hasClient(&client))
        channel->addClient(&client);
    const std::string joinMsg = userPrefix(client) + " JOIN " + channelName + "\r\n";
    server.sendResponse(client.getFd(), joinMsg);
    std::vector<Client*> clients = channel->getClients();
    for (size_t i = 0; i < clients.size(); ++i)
    {
        if (clients[i] && clients[i] != &client)
            server.sendResponse(clients[i]->getFd(), joinMsg);
    }
    if (!channel->getTopic().empty())
    {
        sendNotice(server, client.getFd(), client.getNickname(), "Topic for " + channelName + " is: " + channel->getTopic());
    }
    else
    {
        sendNotice(server, client.getFd(), client.getNickname(), "No topic is set");
    }
    //
    // Need to implement restrictions key(+k), invite only(+i), user limit(+l)
    // First user to join becomes operator(+o)
    // Send appropriate messages for these conditions
    // 
    if (channel->getClientCount() == 1)
        channel->addOp(&client); 
}

void InputParser::handlePart(Client& client, const ParsedInput& parsedInput)
{
    if (!requireRegistration(client, "PART"))
        return ;
    if (parsedInput.args.empty())
    {
        sendError(server, client.getFd(), client.getNickname(), "No channel name given");
        return ;
    }
    std::string channelName = parsedInput.args[0];

    Channel* channel = server.findChannelByName(channelName);
    if (!channel)
    {
        sendError(server, client.getFd(), client.getNickname(), "No such channel " + channelName);
        return ;
    }
    if (!channel->hasClient(&client))
    {
        sendError(server, client.getFd(), client.getNickname(), "You're not on that channel " + channelName);
        return ;
    }
    std::string partMsg = userPrefix(client) + " PART " + channelName + "\r\n";
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
    if (channel->isEmpty())
        server.removeChannel(channelName);
    if (channel->isOp(&client))
    {
        std::vector<Client*> remainingClients = channel->getClients();
        if (!remainingClients.empty())
            channel->addOp(remainingClients[0]);
    }
}

void InputParser::handlePass(Client& client, const ParsedInput& parsedInput)
{
    if (parsedInput.args.empty())
    {
        sendError(server, client.getFd(), client.getNickname(), "No password given");
        return ;
    }
    if (client.isRegistered())
    {
        sendError(server, client.getFd(), client.getNickname(), "You are already registered");
        return ;
    }
    std::string password = parsedInput.args[0];
    if (password != server.getPassword())
    {
        sendError(server, client.getFd(), client.getNickname(), "Password incorrect");
        Logger::warning("Client " + client.getNickname() + " provided incorrect password");
        return ;
    }
    client.setPasswordAccepted(true);
    Logger::info("Client " + client.getNickname() + " provided correct password");
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
        if (channels[i]->isEmpty())
            server.removeChannel(channels[i]->getName());
    }
    server.removeClient(client.getFd());                     
}

void InputParser::handleUser(Client& client, const ParsedInput& parsedInput)
{
    const int fd = client.getFd();

    if (client.isRegistered())
    {
        sendError(server, fd, client.getNickname(), "You are already registered");
        return ;
    }
    //need 4 params: <user> <mode> <unused> :<realname>
    if (parsedInput.args.size() < 4)
    {
        sendError(server, fd, client.getNickname(), "Not enough parameters");
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
        sendSuccess(server, fd, client.getNickname(), " Registration successful");
        sendNotice(server, fd, client.getNickname(), " You registered to " + server.getServerName() + ", " + client.getNickname() + "!");
        sendNotice(server, fd, client.getNickname(), " Your host is " + server.getServerName());
        Logger::info("Client " + client.getNickname() + " has registered successfully");
    }
    else if (!passProvided)
    {
        sendWarning(server, fd, client.getNickname(), "Password required. Please provide the correct password using the PASS command.");
    }
    else if (client.getNickname().empty())
    {
        sendWarning(server, fd, client.getNickname(), "Please set your nickname using the NICK command.");
    }
    
}

void InputParser::handlePrivMsg(Client& client, const ParsedInput& parsedInput)
{
    if (!requireRegistration(client, "PRIVMSG"))
        return ;

    const int fd = client.getFd();
    if (parsedInput.args.size() < 2)
    {
        sendError(server, fd, client.getNickname(), "Not enough parameters for PRIVMSG");
        return ;
    }
    std::string target = parsedInput.args[0];
    std::string message = parsedInput.args[1];
    if (message.empty())
    {
        sendError(server, fd, client.getNickname(), "No text to send");
        return ;
    }
    if (!target.empty() && target[0] == '#')
    {
        Channel* channel = server.findChannelByName(target);
        if (!channel)
        {
            sendError(server, fd, client.getNickname(), "No such channel " + target);
            return ;
        }
        if (!channel->hasClient(&client))
        {
            sendError(server, fd, client.getNickname(), "Can not send to channel " + target);
            return ;
        }
        const std::vector<Client*> clients = channel->getClients();
        for (size_t i = 0; i < clients.size(); ++i)
        {
           Client* targetClient = clients[i];
           if (targetClient && targetClient->getFd() != fd)
           {
               server.sendResponse(targetClient->getFd(), userPrefix(client) + " PRIVMSG " + target + " :" + message + "\r\n");
           }
        }
    }
    else
    {
        Client* directClient = server.getClientByNick(target);
        if (!directClient)
        {
            sendError(server, fd, client.getNickname(), "No such nick/channel " + target);
            return ;
        }
        server.sendResponse(directClient->getFd(), userPrefix(client) + " PRIVMSG " + target + " :" + message + "\r\n");
    }
}

void InputParser::handleNick(Client& client, const ParsedInput& parsedInput)
{
    const int fd = client.getFd();
    const std::string& hostShown = server.getServerName();

    if (parsedInput.args.empty())
    {
        sendError(server, fd, client.getNickname(), "No nickname given");
        return ;
    }
    std::string newNick = parsedInput.args[0];
    static const std::regex nickRe("^[A-Za-z][A-Za-z0-9\\-_]{0,15}$"); //will need to change this logic
    if (!std::regex_match(newNick, nickRe))
    {
        sendError(server, fd, client.getNickname(), "Erroneous nickname");
        return ;
    }

    if (Client* existingClient = server.getClientByNick(newNick))
        if (existingClient && existingClient->getFd() != fd)
    {
        sendError(server, fd, client.getNickname(), "Nickname is already in use");
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
    if (!requireRegistration(client, "INVITE"))
        return ;
    if (parsedInput.args.size() < 2)
    {
        sendError(server, client.getFd(), client.getNickname(), "Not enough parameters");
        return ;
    }
    const std::string targetNick = parsedInput.args[0];
    const std::string channelName = parsedInput.args[1];
    Channel* chan = server.findChannelByName(channelName);
    if (!chan)
    {
        sendError(server, client.getFd(), client.getNickname(), "No such channel");
        return ;
    }
    if (!chan->hasClient(&client))
    {
        sendError(server, client.getFd(), client.getNickname(), "You're not on that channel");
        return ;
    }
    if (chan->isInviteOnly() && !chan->isOp(&client))
    {
        sendError(server, client.getFd(), client.getNickname(), "You're not channel operator");
        return ;
    }

    Client* targetClient = server.getClientByNick(targetNick);
    if (!targetClient)
    {
        sendError(server, client.getFd(), client.getNickname(), "No such nick/channel");
        return ;
    }
    chan->inviteNick(targetNick);
    server.sendResponse(client.getFd(), ":" + server.getServerName() + " " + client.getNickname() + " :You have invited " + targetNick + " to " + channelName + "\r\n");
    server.sendResponse(targetClient->getFd(), ":" + client.getNickname() + " INVITE " + targetNick + " :" + channelName + "\r\n");
}

void InputParser::handleKick(Client& client, const ParsedInput& parsedInput)
{
    if (!requireRegistration(client, "KICK"))
        return ;
    if (parsedInput.args.size() < 2)
    {
        sendError(server, client.getFd(), client.getNickname(), "Not enough parameters");
        return ;
    }
    const std::string nickToKick = parsedInput.args[1];
    Channel* channelName = server.findChannelByName(parsedInput.args[0]);
    if (!channelName)
    {
        sendError(server, client.getFd(), client.getNickname(), "No such channel");
        return ;
    }
    if (!channelName->isOp(&client))
    {
        sendError(server, client.getFd(), client.getNickname(), "You're not channel operator");
        return ;
    }
    Client* targetClient = server.getClientByNick(nickToKick);
    if (!targetClient || !channelName->hasClient(targetClient))
    {
        sendError(server, client.getFd(), client.getNickname(), "No such nick/channel");
        return ;
    }
    const std::string reason = (parsedInput.args.size() > 2) ? parsedInput.args[2] : "No reason specified";
    const std::string kickMsg = ":" + client.getNickname() + " KICK " + channelName->getName() + " " + nickToKick + " :" + reason + "\r\n";
    for (Client* m : channelName->getClients())
    {
        if (m)
            server.sendResponse(m->getFd(), kickMsg);
    }
    channelName->removeClient(targetClient);
    if (channelName->isEmpty())
    {
        server.removeChannel(channelName->getName());
    }
}

void InputParser::handleTopic(Client& client, const ParsedInput& parsedInput)
{
    if (!requireRegistration(client, "TOPIC"))
        return ;
    if (parsedInput.args.empty())
    {
        sendError(server, client.getFd(), client.getNickname(), "No channel name given");
        return ;
    }
    const std::string channelName = parsedInput.args[0];
    Channel* chan = server.findChannelByName(channelName);
    if (!chan)
    {
        sendError(server, client.getFd(), client.getNickname(), "No such channel");
        return ;
    }
    if (!chan->hasClient(&client))
    {
        sendError(server, client.getFd(), client.getNickname(), "You're not on that channel");
        return ;
    }
    if (parsedInput.args.size() == 1)
    {
        if (chan->getTopic().empty())
        {
            sendNotice(server, client.getFd(), client.getNickname(), "No topic is set");
        }
        else
        {
            sendNotice(server, client.getFd(), client.getNickname(), "Topic for " + channelName + " is: " + chan->getTopic());
        }
        return ;
    }
    if (chan->topicOp() && !chan->isOp(&client))
    {
        sendError(server, client.getFd(), client.getNickname(), "You're not channel operator");
        return ;
    }
    chan->setTopic(parsedInput.args[1]);
    server.broadcastToChannel(&client, channelName, ":" + client.getNickname() + " TOPIC " + channelName + " :" + chan->getTopic() + "\r\n");
}

void InputParser::handleMode(Client& client, const ParsedInput& parsedInput)
{
    if (!requireRegistration(client, "MODE"))
        return ;
    if (parsedInput.args.empty())
    {
        sendError(server, client.getFd(), client.getNickname(), "Not enough parameters");
        return ;
    }

    const std::string channel = parsedInput.args[0];
    if (channel.empty() || (channel[0] != '#' && channel[0] != '&'))
    {
        sendError(server, client.getFd(), client.getNickname(), "Erroneous channel name");
        return ;
    }

    Channel* chan = server.findChannelByName(channel);
    if (!chan)
    {
        sendError(server, client.getFd(), client.getNickname(), "No such channel");
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
        sendError(server, client.getFd(), client.getNickname(), "You're not channel operator");
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
                        sendWarning(server, client.getFd(), client.getNickname(), "Channel already has a key");
                        return ;
                    }
                    if (argIndex >= parsedInput.args.size())
                    {
                        sendWarning(server, client.getFd(), client.getNickname(), "Key required");
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
                        sendWarning(server, client.getFd(), client.getNickname(), "User limit required");
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
                    sendWarning(server, client.getFd(), client.getNickname(), "User nickname required");
                    return ;
                }
                const std::string opNick = parsedInput.args[argIndex++];
                Client* opClient = server.getClientByNick(opNick);
                if (!opClient || !chan->hasClient(opClient))
                {
                    sendError(server, client.getFd(), client.getNickname(), "No such nick/channel");
                    return ;
                }
                if (adding)
                    chan->addOp(opClient);
                else
                    chan->removeOp(opClient);
                break ;
            }
            default:
                sendError(server, client.getFd(), client.getNickname(), "Unknown mode flag");
                return ;
        }
    }
    server.broadcastToChannel(&client, channel, ":" + client.getNickname() + " MODE " + channel + " :" + flags + "\r\n");
}
