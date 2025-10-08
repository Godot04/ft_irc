
#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <vector>
#include <string>

class Server;
class Client;

class Command {
protected:
    Server*     _server;
    Client*     _client;
    std::vector<std::string> _params;

public:
    Command(Server* server, Client* client, const std::vector<std::string>& params);
    virtual ~Command();
    virtual void execute() = 0;
};

#endif
