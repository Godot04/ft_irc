#include "../inc/ft_irc.hpp"
#include "../inc/Command.hpp"
#include "../inc/Server.hpp"
#include "../inc/Client.hpp"
#include "../inc/Channel.hpp"
#include "../inc/Reply.hpp"

Command::Command(Server* server, Client* client, const std::string& command, std::istringstream& iss)
    : _server(server), _client(client), _command(command), _iss(iss), _channels(_server->getChannels())
{
}

Command::~Command()
{
}

void Command::executeCommands()
{
    if (_command == "PRIVMSG")
        processPrivmsg();
    else if (_command == "JOIN")
        processJoin();
    // else if (command == "INVITE")
    //     processInvite(client, iss);
    else if (_command == "TOPIC")
        processTopic();
    else
        Reply::unknownCommand(*_client, _command);
}

void Command::processTopic()
{
    std::string target;
    _iss >> target; // extract channel name
    if (target.empty())
    {
        _client->sendMessage("server 461: sent not enough parameters for TOPIC\r\n");
        return ;
    }
    if (target[0] != '#' || target.length() < 2)
    {
        _client->sendMessage("server 403: sent invalid characters for channel name to TOPIC\r\n");
        return ;
    }
    Channel* channel;
    if (_channels.find(target) == _channels.end())
    {
        _client->sendMessage("This channel doesn't exist!\r\n");
        return ;
    }
    channel = _channels[target];
    if (!channel->isClientInChannel(_client))
    {
        _client->sendMessage("You don't have access to this channel!\r\n");
        return ;
    }
    std::string new_topic;
    std::getline(_iss, new_topic);
    while (!new_topic.empty() && isspace(new_topic[0]))
        new_topic = new_topic.substr(1);
    if (new_topic.empty())
    {
        if (!channel->getTopic().empty())
            _client->sendMessage(channel->getTopic() + "\r\n");
        else
            _client->sendMessage("Topic of this channel is not set yet\r\n");
        return ;
    }
    if (!channel->isOperator(_client))
    {
        _client->sendMessage("You don't have operator rights to change the topic\r\n");
        return ;
    }
    channel->setTopic(new_topic);
    std::string formatted_msg = _client->getNickname() + "!" + _client->getUsername() + "@"
                                    + _client->getHostname() + " set new topic for channel " + target + " :"
                                    + new_topic + "\r\n";
    channel->broadcast(formatted_msg, _client);
}

// void Server::processInvite(Client *client, std::istringstream &iss)
// {
//     std::string target;
//     iss >> target; // extract user
//      if (target.empty())
//     {
//         client->sendMessage("server 461: sent not enough parameters for INVITE\r\n");
//         return ;
//     }
// }

void Command::processJoin()
{
    bool the_first_one = false;
    std::string target;
    _iss >> target; // extract channel name
    if (target.empty())
    {
        _client->sendMessage("server 461: sent not enough parameters for JOIN\r\n");
        return ;
    }
    if (target[0] != '#' || target.length() < 2)
    {
        _client->sendMessage("server 403: sent invalid characters for channel name to JOIN\r\n");
        return ;
    }
    Channel *channel;
    if (_channels.find(target) == _channels.end())
    {
        channel = new Channel(target);
        _channels[target] = channel;
        the_first_one = true;
        std::cout << "New channel was created: " << target << " by user " << _client->getNickname() << std::endl;
    }
    else
        channel = _channels[target];
    if (channel->isClientInChannel(_client))
    {
        _client->sendMessage("You're already in this channel\r\n");
        return ;
    }
    if (the_first_one)
        channel->addOperator(_client);
    else
        channel->addClient(_client);
    _client->addChannel(target);
    std::string formatted_msg = _client->getNickname() + "!" + _client->getUsername() + "@"
                                    + _client->getHostname() + " JOIN " + target + "\r\n";
    _client->sendMessage("Welcome to " + target + " channel!\r\n");
    channel->broadcast(formatted_msg, _client);
    if (!channel->getTopic().empty())
        _client->sendMessage(channel->getTopic() + "\r\n");
    else
        _client->sendMessage("Topic of this channel is not set yet\r\n");
    // Send name list (wrote by bot and need for debug)
    std::string names = ":server 353 " + _client->getNickname() + " = " + target + " :";
    const std::vector<Client*>& clients = channel->getClients();
    for (size_t i = 0; i < clients.size(); ++i)
    {
        names += clients[i]->getNickname();
        if (i < clients.size() - 1)
            names += " ";
    }
    _client->sendMessage(names + "\r\n");
    _client->sendMessage(":server 366 " + _client->getNickname() + " " + target + " :End of NAMES list\r\n");
}

void    Command::processPrivmsg()
{
    std::string target;
    _iss >> target; // extract channel or nickname
    std::string message;
    std::getline(_iss, message);
    if (target.empty() || message.empty())
    {
        _client->sendMessage("server 461: " + _client->getNickname() + " sent not enough parameters (PRIVMSG)\r\n");
        return ;
    }
    std::cout << "PRIVMSG from " << _client->getNickname() << " to " << target << ": " << message << std::endl;
    if (target[0] == '#')
    {
        if (_channels.find(target) == _channels.end())
        {
            _client->sendMessage("server 403: " + _client->getNickname() + " sent message to " + target + " :No such channel exist\r\n");
            return ;
        }
        Channel *channel = _channels[target];
        if (!channel->isClientInChannel(_client))
        {
            _client->sendMessage("server 404: " + _client->getNickname() + " doesn't have acces to this channel - " + target);
            return ;
        }
        std::string formatted_msg = _client->getNickname() + "!" + _client->getUsername() + "@"
                                    + _client->getHostname() + " sent message to channel " + target + " :"
                                    + message + "\r\n";
        channel->broadcast(formatted_msg, _client);
    }
    else
    {
        std::map<int, Client*>& clients = _server->getClients();
        Client *target_user = NULL;
        for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); it++)
        {
            if (it->second->getNickname().compare(target) == 0)
            {
                target_user = it->second;
                break;
            }
        }
        if (target_user == NULL)
        {
            _client->sendMessage("server 401: " + target + " doesn't exist (sent by " + _client->getNickname() + ")" );
            return ;
        }
        std::string formatted_msg = _client->getNickname() + "!" + _client->getUsername() + "@"
                                    + _client->getHostname() + " sent message to user " + target + " :"
                                    + message + "\r\n";
        target_user->sendMessage(formatted_msg);
    }
}
