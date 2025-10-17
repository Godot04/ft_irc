#include <ChannelsClientsManager.hpp>


ChannelsClientsManager::ChannelsClientsManager() {}

ChannelsClientsManager::~ChannelsClientsManager() {}


void ChannelsClientsManager::setClientsMap(std::map<int, Client*> *clients, std::string const *password, std::vector<pollfd> *pollfds) {
	_clients = clients;
	_password = password;
	_pollfds = pollfds;
}

void ChannelsClientsManager::handleClientMessage(Client* client) {
	// Parse the client's message from its buffer
	// std::string message = client->getBuffer();

	for (std::string message = client->getNextMessage(); !message.empty(); message = client->getNextMessage())
	{
		message += "\r\n"; // Add CRLF back for parsing
		IRCCommand command(message);
		if (!command.isValid()) {
			client->sendMessage("TEMP_ERR :Invalid command format\r\n");
			return;
		}
		else {
			if (!client->isRegistered()) {
				registerClient(client, command);
			}
			else {
				handleRegisteredClientMessage(client, command);
			}

		}
	}
}

void ChannelsClientsManager::handleRegisteredClientMessage(Client* client, IRCCommand& command)
{
	if (command.getCommand() == "PASS" || command.getCommand() == "USER")
		Reply::alreadyRegistered(*client);
	else if (command.getCommand() == "NICK")
	{
		// std::string newNick = command.getParams().at(0);
		// if (isNickInUse(newNick)) {
		// 	Reply::nicknameInUse(*client, newNick);
		// }
		// else {
		// 	client->setNickname(newNick);
		// }
	}
	else if (command.getCommand() == "JOIN")
		executeJoin(client, command);
	else if (command.getCommand() == "PRIVMSG")
		executePrivmsg(client, command);
	else if (command.getCommand() == "INVITE")
		executeInvite(client, command);
	else if (command.getCommand() == "TOPIC")
		executeTopic(client, command);
	else if (command.getCommand() == "KICK")
		executeKick(client, command);
	else
		Reply::unknownCommand(*client, command.getCommand());
}

bool ChannelsClientsManager::isNickInUse(const std::string& nickname) const {
	for (std::map<int, Client*>::iterator it = _clients->begin(); it != _clients->end(); it++)
	{
		if (it->second->getNickname() == nickname)
			return true;
	}
	return false;
}

void ChannelsClientsManager::registerClient(Client* client, IRCCommand& command) {
if (command.getCommand() == "PASS") {
	if (command.getParams().at(0) == *(_password)) {
		client->setAuthenticated(true);
	}
	else {
		Reply::passwordMismatch(*client);
		return;
	}
	}
	else if (command.getCommand() == "NICK") {
		// Check if nickname is already in use
		std::string nickname = command.getParams().at(0);
		if (isNickInUse(nickname)) {
			Reply::nicknameInUse(*client, nickname);
			return;
		}
		else {
			client->setNickname(nickname);
		}
	}
	else if (command.getCommand() == "USER") {
		std::string username = command.getParams().at(0);
		std::string realname = command.getParams().at(3);
		if (!realname.empty() && realname[0] == ':')
			realname = realname.substr(1);
		client->setUsername(username);
		client->setRealname(realname);
	}
	else if (command.getCommand() == "CAP") {
		// For simplicity, we just acknowledge CAP commands without actual negotiation
		std::string serverName = SERVER_NAME;
		if (command.getParams().at(0) == "LS") {
			client->sendMessage(serverName + " CAP * LS :\r\n");
			client->setCAPNegotiation(true);
		}
		else if (command.getParams().at(0) == "ACK") {
			client->sendMessage(serverName + " CAP * ACK :\r\n");
			client->setCAPNegotiation(true);
		}
		else if (command.getParams().at(0) == "END") {
			client->setCAPNegotiation(false);
		}
		else {
			Reply::unknownCommand(*client, command.getCommand());
			return;
		}
	}
	else {
		Reply::unknownCommand(*client, command.getCommand());
		return;
	}
	if (client->isAuthenticated() && client->isNicknameSet() && client->isUsernameSet() && !client->isCAPNegotiation()) {
		client->setRegistered(true);
		Reply::welcome(*client);
	}
}

void ChannelsClientsManager::executePrivmsg(Client* client, IRCCommand& command)
{
	const std::vector<std::string>& params = command.getParams();
	std::string target = params[0];
	std::string message = params[1];
	if (target[0] == '#' || target[0] == '&')
	{
		if (_channels.find(target) == _channels.end())
		{
			client->sendMessage("server 403: " + client->getNickname() + " sent message to " + target + " :No such channel exist\r\n");
			return;
		}
		Channel *channel = _channels[target];
		if (!channel->isClientInChannel(client))
		{
			client->sendMessage("server 404: " + client->getNickname() + " doesn't have access to this channel - " + target + "\r\n");
			return;
		}
		std::string formatted_msg = client->getNickname() + "!" + client->getUsername() + "@"
									+ client->getHostname() + " PRIVMSG " + target + " :"
									+ message + "\r\n";
		channel->broadcast(formatted_msg, client);
	}
	else
	{
		Client *target_user;
		if ((target_user = getClientByNickname(target, client)) == NULL)
			return ;
		std::string formatted_msg = client->getNickname() + "!" + client->getUsername() + "@"
									+ client->getHostname() + " PRIVMSG " + target + " :"
									+ message + "\r\n";
		target_user->sendMessage(formatted_msg);
	}
}

void ChannelsClientsManager::executeJoin(Client* client, IRCCommand& command)
{
	const std::vector<std::string>& params = command.getParams();
	std::string channels = params[0];
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
			client->sendMessage("server 403: sent invalid characters for channel name to JOIN\r\n");
			continueLoopJoin(start, end, channels);
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
		if (channel->isClientInChannel(client))
		{
			client->sendMessage("You're already in this channel\r\n");
			continueLoopJoin(start, end, channels);
			continue;
		}
		if (the_first_one)
			channel->addOperator(client);
		else
			channel->addClient(client);
		client->addChannel(target);
		std::string formatted_msg = client->getNickname() + "!" + client->getUsername() + "@"
									+ client->getHostname() + " JOIN " + target + "\r\n";
		client->sendMessage("Welcome to " + target + " channel!\r\n");
		channel->broadcast(formatted_msg, client);
		if (!channel->getTopic().empty())
			client->sendMessage(channel->getTopic() + "\r\n");
		else
			client->sendMessage("Topic of this channel is not set yet\r\n");
		continueLoopJoin(start, end, channels);
	}
}

void ChannelsClientsManager::executeInvite(Client* client, IRCCommand& command)
{
	const std::vector<std::string>& params = command.getParams();
	std::string target_nick = params[0];
	std::string target_channel = params[1];
	Client *target_user;
	if ((target_user = getClientByNickname(target_nick, client)) == NULL)
		return ;
	if (_channels.find(target_channel) == _channels.end())
	{
		client->sendMessage("server 403: This channel doesn't exist!\r\n");
		return;
	}
	Channel *channel = _channels[target_channel];
	if (!channel->isClientInChannel(client))
	{
		client->sendMessage("server 442: You don't have access to this channel!\r\n");
		return;
	}
	if (channel->isClientInChannel(target_user))
	{
		client->sendMessage("server 443: This user is already in channel\r\n");
		return;
	}
	target_user->sendMessage(client->getNickname() + " invited you to this channel: "
								+ channel->getName() + "\r\n");
	client->sendMessage("You're successfully invited " + target_user->getNickname()
						+ " to this channel " + channel->getName() + "\r\n");
}

void ChannelsClientsManager::executeTopic(Client* client, IRCCommand& command)
{
	const std::vector<std::string>& params = command.getParams();
	std::string target = params[0];
	if (target[0] != '#' || target.length() < 2)
	{
		client->sendMessage("server 403: sent invalid characters for channel name to TOPIC\r\n");
		return;
	}
	if (_channels.find(target) == _channels.end())
	{
		client->sendMessage("This channel doesn't exist!\r\n");
		return;
	}
	Channel* channel = _channels[target];
	if (!channel->isClientInChannel(client))
	{
		client->sendMessage("You don't have access to this channel!\r\n");
		return;
	}
	if (params.size() == 1)
	{
		if (!channel->getTopic().empty())
			client->sendMessage(channel->getTopic() + "\r\n");
		else
			client->sendMessage("Topic of this channel is not set yet\r\n");
		return;
	}
	std::string new_topic = params[1];
	if (!channel->isOperator(client))
	{
		client->sendMessage("You don't have operator rights to change the topic\r\n");
		return;
	}
	channel->setTopic(new_topic);
	std::string formatted_msg = client->getNickname() + "!" + client->getUsername() + "@"
								+ client->getHostname() + " TOPIC " + target + " :"
								+ new_topic + "\r\n";
	channel->broadcast(formatted_msg, client);
}

void ChannelsClientsManager::executeKick(Client* client, IRCCommand& command)
{
	const std::vector<std::string>& params = command.getParams();
	std::string target_channel = params[0];
	std::string target_nick = params[1];
	if (target_channel[0] != '#' && target_channel[0] != '&')
	{
		client->sendMessage("Input must follow irc protocol (#channel or &channel)\r\n");
		return;
	}
	if (_channels.find(target_channel) == _channels.end())
	{
		client->sendMessage("server 403: This channel doesn't exist!\r\n");
		return;
	}
	Channel *channel = _channels[target_channel];
	if (!channel->isClientInChannel(client))
	{
		client->sendMessage("server 442: You don't have access to this channel!\r\n");
		return;
	}
	if (!channel->isOperator(client))
	{
		client->sendMessage("server 482: You don't have operator rights to kick client\r\n");
		return;
	}
	Client *target_user;
	if ((target_user = getClientByNickname(target_nick, client)) == NULL)
		return ;
	if (!channel->isClientInChannel(target_user))
	{
		client->sendMessage("server 441: User doesn't have access to this channel\r\n");
		return;
	}
	std::string kick_message;
	if (params.size() >= 3)
		kick_message = params[2];
	else
		kick_message = "No specific reason";
	channel->removeClient(target_user);
	target_user->removeChannel(target_channel);
	std::string formatted_msg = "User " + target_nick + " was kicked from " + target_channel
								+ " by " + client->getNickname()
								+ " (" + kick_message + ") " + "\r\n";
	channel->broadcast(formatted_msg);
	target_user->sendMessage("You were kicked from this server: " + target_channel
							+ " for this reason: " + kick_message + "\r\n");
}

Client* ChannelsClientsManager::getClientByNickname(const std::string& target_nick, Client* client)
{
	Client *target_user = NULL;
	for (std::map<int, Client*>::iterator it = _clients->begin(); it != _clients->end(); it++)
    {
        if (it->second->getNickname() == target_nick)
        {
            target_user = it->second;
            break;
        }
    }
    if (target_user == NULL)
    {
        client->sendMessage("server 401: No sush nick is exist\r\n");
        return NULL;
    }
    return (target_user);
}

void ChannelsClientsManager::continueLoopJoin(size_t &start, size_t &end, const std::string& channels)
{
	if (end == std::string::npos)
		start = channels.length();
	else
	{
		start = end + 1;
		end = channels.find(',', start);
	}
}


Channel* ChannelsClientsManager::getChannel(std::string const &channelName) {
	std::map<std::string, Channel*>::iterator it = _channels.find(channelName);
	if (it != _channels.end())
		return it->second;
	return NULL;
}
