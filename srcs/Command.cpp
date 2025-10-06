#include "../inc/ft_irc.hpp"

Command::Command(Server* server, Client* client, const std::vector<std::string>& params)
    : _server(server), _client(client), _params(params)
{
}

Command::~Command()
{
}