
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
        Server*                          _server;
        Client*                          _client;
        std::string                      _command;
        std::istringstream&              _iss;
        std::map<std::string, Channel*>& _channels;

    public:
        Command(Server* server, Client* client, const std::string& command, std::istringstream& iss);
        ~Command();
        void    executeCommands();
        // Registration Commands
        void processPassword();
        void processNick();
        void processUser();
        void processCAP();
        // Commands for registered users
        void    processPrivmsg();
        void    processJoin();
        void    processInvite();
        void    processTopic();
        void    processKick();
        void    processList();
        // Utils
        void    continue_loop_Join(size_t &start, size_t &end, std::string channels);
        void    clear_space(std::string &message);
};

#endif
