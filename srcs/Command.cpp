#include "../inc/ft_irc.hpp"
#include "../inc/Command.hpp"
#include "../inc/Server.hpp"
#include "../inc/Client.hpp"
#include "../inc/Channel.hpp"
#include "../inc/Reply.hpp"

Command::Command(Server* server, Client* client, const std::string& command, std::istringstream& iss)
    : _server(server), _client(client), _command(command), _iss(iss),
      _channels(_server->getChannels()), _clients(_server->getClients())
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
    else if (_command == "INVITE")
        processInvite();
    else if (_command == "KICK")
        processKick();
    else if (_command == "TOPIC")
        processTopic();
    else if (_command == "LIST")
        processList();
    else
        Reply::unknownCommand(*_client, _command);
}

void Command::processInvite()
{
    std::string target_nick;
    std::string target_channel;
    _iss >> target_nick;
    _iss >> target_channel;
    if (target_nick.empty() || target_channel.empty())
    {
        _client->sendMessage("server 461: sent not enough parameters for INVITE\r\n");
        return ;
    }
    Client *target_user;
    if ((target_user = get_other_nick(target_nick)) == NULL)
        return ;
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
    if (channel->isClientInChannel(target_user))
    {
        _client->sendMessage("server 443: This user is already in channel\r\n");
        return ;
    }
    target_user->sendMessage(_client->getNickname() + " invited you to this channel: "
                                + channel->getName() + "\r\n");
    _client->sendMessage("You're succesfully invited " + target_user->getNickname()
                            + " to this channel " + channel->getName() + "\r\n");
}

void Command::processKick()
{
    std::string target_channel;
    std::string target_nick;
    _iss >> target_channel;
    _iss >> target_nick;
    if (target_channel.empty() || target_nick.empty())
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
        Client *target_user;
        if ((target_user = get_other_nick(target_nick)) == NULL)
            return ;
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
        _client->sendMessage("server 461: sent not enough parameters for PRIVMSG\r\n");
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
        Client *target_user;
        if ((target_user = get_other_nick(target)) == NULL)
            return ;
        std::string formatted_msg = _client->getNickname() + "!" + _client->getUsername() + "@"
                                    + _client->getHostname() + " sent message to user " + target + " :"
                                    + message + "\r\n";
        target_user->sendMessage(formatted_msg);
    }
}
