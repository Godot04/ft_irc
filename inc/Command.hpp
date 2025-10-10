
#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <vector>
#include <string>
#include <sstream>

class Server;
class Client;

class Command
{
    private:
        Server*            _server;
        Client*            _client;
        std::string        _command;
        std::istringstream& _iss;

    public:
        Command(Server* server, Client* client, const std::string& command, std::istringstream& iss);
        ~Command();
        void    executeCommands();
        void    processPrivmsg();
        void    processJoin();
        void    processInvite();
        void    processTopic();
};

#endif
