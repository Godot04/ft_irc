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
