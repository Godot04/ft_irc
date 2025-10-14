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
    else if (_command == "KICK")
        processKick();
    else if (_command == "TOPIC")
        processTopic();
    else if (_command == "LIST")
        processList();
    else
        Reply::unknownCommand(*_client, _command);
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

void Command::processList()
{
    std::map<int, Client*>& clients = _server->getClients();
    int counter = 1;

    // Header
    _client->sendMessage("---- User List ----\r\n");

    for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        Client* user = it->second;

        // Convert counter to string using stringstream (C++98 compatible)
        std::stringstream ss;
        ss << counter;

        std::string output = ss.str() + ". " + user->getNickname() +
                            " (" + user->getUsername() + "): ";

        // Get channels and operator status
        const std::vector<std::string>& userChannels = user->getChannels();
        bool firstChannel = true;

        if (userChannels.empty())
        {
            output += "no channels";
        }
        else
        {
            for (std::vector<std::string>::const_iterator chan = userChannels.begin();
                 chan != userChannels.end(); ++chan)
            {
                if (!firstChannel)
                    output += ", ";
                else
                    firstChannel = false;

                // Check if user is operator in this channel
                Channel* channel = _channels[*chan];
                if (channel && channel->isOperator(user))
                    output += *chan + " (op)";
                else
                    output += *chan;
            }
        }

        _client->sendMessage(output + "\r\n");
        counter++;
    }

    _client->sendMessage("---- End of List ----\r\n");
}

void Command::processKick()
{
    std::string target_channel;
    _iss >> target_channel;
    if (target_channel.empty())
    {
        _client->sendMessage("server 461: sent not enough parameters for KICK\r\n");
        return ;
    }
    if (target_channel[0] == '#' || target_channel[0] == '&')
    {
        if (_channels.find(target_channel) == _channels.end())
        {
            _client->sendMessage("server 403: This channel doesn't exist!\r\n");
            return ;
        }
        Channel *channel = _channels[target_channel];
        if (!channel->isClientInChannel(_client))
        {
            _client->sendMessage("server 442: You don't have access to this channel!\r\n");
            return ;
        }
        if (!channel->isOperator(_client))
        {
            _client->sendMessage("server 482: You don't have operator rights to kick client\r\n");
            return ;
        }
        std::string target_nick;
        _iss >> target_nick;
        if (target_nick.empty())
        {
            _client->sendMessage("server 461: sent not enough parameters for KICK (user)\r\n");
            return ;
        }
        Client *target_user = NULL;
        std::map<int, Client*>& clients = _server->getClients();
        for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); it++)
        {
            if (it->second->getNickname() == target_nick)
            {
                target_user = it->second;
                break;
            }
        }
        if (target_user == NULL)
        {
            _client->sendMessage("server 401: No sush nick is exist\r\n");
            return ;
        }
        if (!channel->isClientInChannel(target_user))
        {
            _client->sendMessage("server 441: User doesn't have acces to this channel\r\n");
            return ;
        }
        channel->removeClient(target_user);
        target_user->removeChannel(target_channel);
        std::string kick_message;
        std::getline(_iss, kick_message);
        clear_space(kick_message);
        if (kick_message.empty())
            kick_message = "No specific reason";
        std::string formatted_msg = "User " + target_nick + " was kicked from " + target_channel
                                        + " by " + _client->getNickname()
                                        + " (" + kick_message + ") " + "\r\n";
        channel->broadcast(formatted_msg);
        target_user->sendMessage("You were kicked from this server: " + target_channel
                                    + " for this reason: " + kick_message + "\r\n");
    }
    else
    {
        _client->sendMessage("Input must follow irc protocol (#channel or &channel)\r\n");
        return ;
    }
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
    clear_space(new_topic);
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

void Command::processJoin()
{
    std::string channels;
    _iss >> channels;
    if (channels.empty())
    {
        _client->sendMessage("server 461: sent not enough parameters for JOIN\r\n");
        return ;
    }
    size_t start = 0;
    size_t end = channels.find(',');
    while (start < channels.length())
    {
        std::string target;
        if (end == std::string::npos)
            target = channels.substr(start);
        else
            target = channels.substr(start, end - start);
        if ((target[0] != '#' && target[0] != '&') || target.length() < 2)
        {
            _client->sendMessage("server 403: sent invalid characters for channel name to JOIN\r\n");
            continue_loop_Join(start, end, channels);
            continue;
        }
        bool the_first_one = false;
        Channel *channel;
        if (_channels.find(target) == _channels.end())
        {
            channel = new Channel(target);
            _channels[target] = channel;
            the_first_one = true;
        }
        else
            channel = _channels[target];
        if (channel->isClientInChannel(_client))
        {
            _client->sendMessage("You're already in this channel\r\n");
            continue_loop_Join(start, end, channels);
            continue;
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
        continue_loop_Join(start, end, channels);
    }
}

void    Command::processPrivmsg()
{
    std::string target;
    _iss >> target; // extract channel or nickname
    std::string message;
    std::getline(_iss, message);
    clear_space(message);
    if (target.empty() || message.empty())
    {
        _client->sendMessage("server 461: " + _client->getNickname() + " sent not enough parameters (PRIVMSG)\r\n");
        return ;
    }
    if (target[0] == '#' || target[0] == '&')
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
