#pragma once

#include <sstream>
#include <cctype>
#include <string>
#include <vector>
#include <unordered_map>

class Server;
class Client;

struct ParsedInput {
    std::string command;
    std::vector<std::string> args;
    std::string prefix;
};

class InputParser {
    public:
        InputParser(Server& server);

        static ParsedInput parseLine(const std::string& line);
        void processInput(int fd, const std::string& bytes);
        void handleCommand(Client& client, const ParsedInput& parsedInput);

        // void onClientDisconnect(Client& client);

    private:
        Server& server;
        std::unordered_map<int, std::string> clientBuffers;
 
        typedef void (InputParser::*CommandHandler)(Client&, const ParsedInput&);
        std::unordered_map<std::string, CommandHandler> commandHandlers;
        // Command Handlers
        void registerHandlers();
        void handleJoin(Client& client, const ParsedInput& parsedInput);
        void handlePart(Client& client, const ParsedInput& parsedInput);
        // void handlePrivMsg(Client& client, const ParsedInput& parsedInput);
        // void handleNick(Client& client, const ParsedInput& parsedInput);
        // void handleUser(Client& client, const ParsedInput& parsedInput);
        // void handlePass(Client& client, const ParsedInput& parsedInput);
        // void handlePing(Client& client, const ParsedInput& parsedInput);
        // void handlePong(Client& client, const ParsedInput& parsedInput);
        // void handleQuit(Client& client, const ParsedInput& parsedInput);
        // void handleMode(Client& client, const ParsedInput& parsedInput);
        // void handleTopic(Client& client, const ParsedInput& parsedInput);
        // void handleInvite(Client& client, const ParsedInput& parsedInput);
        // void handleKick(Client& client, const ParsedInput& parsedInput);
        // void handleUnknownCommand(Client& client, const ParsedInput& parsedInput);
};