#include "../inc/ft_irc.hpp"
#include "../inc/Command.hpp"
#include "../inc/Server.hpp"
#include "../inc/Client.hpp"
#include "../inc/Channel.hpp"
#include "../inc/Reply.hpp"

void Command::continue_loop_Join(size_t &start, size_t &end, std::string channels)
{
	if (end == std::string::npos)
        start = channels.length();
    else
    {
        start = end + 1;
        end = channels.find(',', start);
    }
}

void Command::clear_space(std::string &message)
{
    while (!message.empty() && isspace(message[0]))
        message = message.substr(1);
    if (!message.empty() && message[0] == ':')
        message = message.substr(1);
}

Client* Command::get_other_nick(std::string target_nick)
{
    Client *target_user = NULL;
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); it++)
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
        return NULL;
    }
    return (target_user);
}

// for debug
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
