#include "../inc/ft_irc.hpp"

Channel::Channel(const std::string& name)
    : _name(name), _topic(""), _isInviteOnly(false), _topicProtected(false), _key(""), _userLimit(0)
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
            std::cout << "removed addres: " + client->getNickname() << std::endl;
            break;
        }
    }

    // Remove from operators
    for (std::vector<Client*>::iterator it = _operators.begin(); it != _operators.end(); ++it)
    {
        if (*it == client)
        {
            std::cout << "removed operator: " + client->getNickname() << std::endl;
            _operators.erase(it);
            break;
        }
    }

    // Remove from invited list
    for (std::vector<Client*>::iterator it = _invited.begin(); it != _invited.end(); ++it)
    {
        if (*it == client)
        {
            std::cout << "removed invited: " + client->getNickname() << std::endl;
            _invited.erase(it);
            break;
        }
    }

    // Remove channel from client's channel list
    for (std::vector<std::string>::iterator it = client->getChannels().begin(); it != client->getChannels().end(); ++it)
    {
        if (*it == _name)
        {
            std::cout << "removed channel: " + client->getNickname() << std::endl;
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

bool Channel::isClientInChannel(const std::string& client) const
{
    for (std::vector<Client*>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if ((*it)->getNickname() == client)
            return true;
    }
    return false;
}

void Channel::removeClient(const std::string& client)
{
    for (std::vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if ((*it)->getNickname() == client)
        {
            Client* cl = *it;
            _clients.erase(it);
            // Also remove from operators if present
            removeOperator(cl);
            // Remove channel from client's channel list
            for (std::vector<std::string>::iterator it2 = cl->getChannels().begin(); it2 != cl->getChannels().end(); ++it2)
            {
                if (*it2 == _name)
                {
                    cl->getChannels().erase(it2);
                    break;
                }
            }
        }
    }
}

void Channel::setInviteOnly(bool status) {
    _isInviteOnly = status;
}

bool Channel::isInviteOnly() const {
    return _isInviteOnly;
}

void Channel::addInvited(Client* client)
{
    if (!client)
        return;
    if (isInvited(client))
        return;
    _invited.push_back(client);
}

void Channel::removeInvited(Client* client)
{
    if (!client)
        return;
    for (std::vector<Client*>::iterator it = _invited.begin(); it != _invited.end(); ++it)
    {
        if (*it == client)
        {
            _invited.erase(it);
            break;
        }
    }
}

bool Channel::isInvited(Client* client) const
{
    if (!client)
        return false;
    for (std::vector<Client*>::const_iterator it = _invited.begin(); it != _invited.end(); ++it)
    {
        if (*it == client)
            return true;
    }
    return false;
}
