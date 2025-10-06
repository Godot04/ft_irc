#include "../inc/ft_irc.hpp"

Channel::Channel(const std::string& name)
    : _name(name)
{
}

Channel::~Channel()
{
}

void Channel::addClient(Client* client)
{
    // Check if client is already in channel
    if (isClientInChannel(client))
        return;
    
    _clients.push_back(client);
    client->getChannels().push_back(_name);
}

void Channel::removeClient(Client* client)
{
    // Remove from clients
    for (std::vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (*it == client)
        {
            _clients.erase(it);
            break;
        }
    }
    
    // Remove from operators
    for (std::vector<Client*>::iterator it = _operators.begin(); it != _operators.end(); ++it)
    {
        if (*it == client)
        {
            _operators.erase(it);
            break;
        }
    }
    
    // Remove channel from client's channel list
    for (std::vector<std::string>::iterator it = client->getChannels().begin(); it != client->getChannels().end(); ++it)
    {
        if (*it == _name)
        {
            client->getChannels().erase(it);
            break;
        }
    }
}

void Channel::addOperator(Client* client)
{
    // Check if client is already an operator
    if (isOperator(client))
        return;
    
    // Check if client is in channel
    if (!isClientInChannel(client))
        addClient(client);
    
    _operators.push_back(client);
}

void Channel::removeOperator(Client* client)
{
    for (std::vector<Client*>::iterator it = _operators.begin(); it != _operators.end(); ++it)
    {
        if (*it == client)
        {
            _operators.erase(it);
            break;
        }
    }
}

void Channel::broadcast(const std::string& message, Client* sender)
{
    for (std::vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (*it != sender)
            (*it)->sendMessage(message);
    }
}

bool Channel::isClientInChannel(Client* client) const
{
    for (std::vector<Client*>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (*it == client)
            return true;
    }
    return false;
}

bool Channel::isOperator(Client* client) const
{
    for (std::vector<Client*>::const_iterator it = _operators.begin(); it != _operators.end(); ++it)
    {
        if (*it == client)
            return true;
    }
    return false;
}

const std::string& Channel::getName() const
{
    return _name;
}

const std::string& Channel::getTopic() const
{
    return _topic;
}

void Channel::setTopic(const std::string& topic)
{
    _topic = topic;
}

std::vector<Client*>& Channel::getClients()
{
    return _clients;
}

std::vector<Client*>& Channel::getOperators()
{
    return _operators;
}