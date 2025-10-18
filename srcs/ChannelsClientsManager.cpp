#include <ChannelsClientsManager.hpp>


ChannelsClientsManager::ChannelsClientsManager(std::map<int, Client*> &clients, std::string const &password, std::vector<pollfd> &pollfds)
	: _clients(clients), _password(password), _pollfds(pollfds)
{}

ChannelsClientsManager::~ChannelsClientsManager() {}


// void ChannelsClientsManager::setClientsMap(std::map<int, Client*> *clients, std::string const *password, std::vector<pollfd> *pollfds) {
// 	// _clients. clients;
// 	// _password = password;
// 	// _pollfds = pollfds;
// }

void ChannelsClientsManager::handleClientMessage(Client* client) {
	// Parse the client's message from its buffer
	// std::string message = client->getBuffer();

	for (std::string message = client->getNextMessage(); !message.empty(); message = client->getNextMessage())
	{
		// std::cout << "MESSAGE: "<< message << std::endl;
		message += "\r\n"; // Add CRLF back for parsing
		IRCCommand command(message);
		if (!command.isValid()) {
			client->sendMessage("TEMP_ERR :Invalid command format\r\n");
			return;
		}
		else {
			client->updateConnectionTime();
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
		std::string newNick = command.getParams().at(0);
		if (isNickInUse(newNick)) {
			Reply::nicknameInUse(*client, newNick);
		}
		else {
			client->setNickname(newNick);
		}
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
	else if (command.getCommand() == "PING") {
		executePing(client, command);
	}
	else if (command.getCommand() == "PONG") {
		// Just update the connection time
		client->updateConnectionTime();
	}
	else if (command.getCommand() == "MODE") {
		executeMode(client, command);
	}
	else if (command.getCommand() == "WHOIS") {
		executeWhois(client, command);
	}
	else
		Reply::unknownCommand(*client, command.getCommand());
}

void ChannelsClientsManager::executeMode(Client* client, IRCCommand& command) {
	if (command.getParamsCount() < 1) {
		client->sendMessage("server 461: Not enough parameters for MODE\r\n");
		return;
	}
	std::string targetChannel = command.getParamAt(0);
	if (targetChannel[0] == '#' || targetChannel[0] == '&') {
		if (_channels.find(targetChannel) == _channels.end()) {
			Reply::noSuchChannel(*client, targetChannel);
			return;
		}
		Channel *channel = _channels[targetChannel];
		if (command.getParamsCount() < 2) {
			std::string modes = "+";
			if (channel->isInviteOnly()) modes += "i";
			if (channel->isTopicProtected()) modes += "t";
			if (!channel->getKey().empty()) modes += "k";
			if (channel->getUserLimit() > 0) modes += "l";
			client->sendMessage(":" + std::string(SERVER_NAME) + " 324 " + client->getNickname() +
				" " + targetChannel + " " + modes + "\r\n");
			return;
		}
		if (!channel->isOperator(client)) {
			Reply::notOperator(*client, targetChannel);
			return;
		}
		handleModeFlags(*client, *channel, command);
	}
}

void ChannelsClientsManager::handleModeFlags(Client &client, Channel &channel, IRCCommand& command) {
	std::string modeChanges = command.getParamAt(1);
	// std::string modeParams = (command.getParamsCount() > 2) ? command.getParamAt(2) : "";
	std::vector<std::string> modeParams;
	std::string modeParamsString;
	for (size_t i = 2; i < command.getParamsCount(); ++i) {
		modeParams.push_back(command.getParamAt(i));
		modeParamsString += " " + command.getParamAt(i);
	}
	ModeSign currentSign = NONE;
	size_t paramIndex = 0;
	// 0 -> channel, 1 -> modeChanges, 2 -> modeParams
	for (size_t i = 0; i < modeChanges.length(); ++i) {
		char c = modeChanges[i];
		if (c == '+') {
			currentSign = PLUS;
		} else if (c == '-') {
			currentSign = MINUS;
		} else {
			ModeFlag flag = IRCCommand::charToModeFlag(c);
			switch (flag) {
				case MODE_INVITE:
					if (currentSign == PLUS) {
						channel.setInviteOnly(true);
					} else if (currentSign == MINUS) {
						channel.setInviteOnly(false);
					}
					break;
				case MODE_TOPIC:
					if (currentSign == PLUS) {
						channel.setTopicProtected(true);
					} else if (currentSign == MINUS) {
						channel.setTopicProtected(false);
					}
					break;
				case MODE_KEY: {
					if (paramIndex >= modeParams.size()) {
                        client.sendMessage(":" + std::string(SERVER_NAME) + " 461 " + client.getNickname() + " MODE :Not enough parameters\r\n");
                        break;
                    }
					std::string key = modeParams[paramIndex];
					channel.setKey(key);
					paramIndex++;
					break;
				}
				case MODE_LIMIT_USER: {
					if (paramIndex >= modeParams.size()) {
						client.sendMessage(":" + std::string(SERVER_NAME) + " 461 " + client.getNickname() + " MODE :Not enough parameters\r\n");
						break;
					}
					size_t limit = static_cast<size_t>(atoi(modeParams[paramIndex].c_str()));
					channel.setUserLimit(limit);
					paramIndex++;
					break;
				}
				case MODE_OPERATOR: {
					if (paramIndex >= modeParams.size()) {
						client.sendMessage(":" + std::string(SERVER_NAME) + " 461 " + client.getNickname() + " MODE :Not enough parameters\r\n");
						break;
					}
					std::string target = modeParams[paramIndex];
					if (currentSign == MINUS) {
						// Remove operator
						Client* targetClient = getClientByNickname(target, &client);
						if (targetClient && channel.isClientInChannel(targetClient)) {
							channel.removeOperator(targetClient);
							// channel.broadcast(":" + client.getNickname() + " MODE " + channel.getName() + " -o " + target + "\r\n", &client);
						} else {
							Reply::usersDontMatch(client);
						}
					}
					else if (currentSign == PLUS) {
						// Add operator
						Client* targetClient = getClientByNickname(target, &client);
						if (!targetClient || !channel.isClientInChannel(targetClient)) {
							Reply::usersDontMatch(client);
							break;
						}
						channel.addOperator(targetClient);
						// channel.broadcast(":" + client.getNickname() + " MODE " + channel.getName() + " +o " + target + "\r\n", &client);
					}
					paramIndex++;
					break;
				}
				default:
					Reply::unknownCommand(client, "MODE");
					// client.sendMessage(":" + SERVER_NAME + " 472 " + client.getNickname() + " " + c + " :is unknown mode char to me\r\n");
					break;
			}
		}
	}
	// Notify the channel about the mode change

	channel.broadcast(":" + std::string(SERVER_NAME) + " " + std::string(RPL_CHANNELMODEIS) + " " + client.getNickname() + " " + channel.getName() + " :" + modeChanges + modeParamsString + "\r\n", &client);
}

void ChannelsClientsManager::executePing(Client* client, IRCCommand& command) {
	std::cout << "Executing PING command " << command.getParams().at(0) << std::endl;
	Reply::pongReply(*client, command.getParams().at(0));
}

void ChannelsClientsManager::executeWhois(Client* client, IRCCommand& command) {
	if (command.getParamsCount() < 1) {
		client->sendMessage(":" + std::string(SERVER_NAME) + " 431 " + client->getNickname() + " :No nickname given\r\n");
		return;
	}

	std::string targetNick = command.getParamAt(0);
	Client* targetClient = NULL;

	// Find the target client
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		if (it->second && it->second->getNickname() == targetNick) {
			targetClient = it->second;
			break;
		}
	}

	if (!targetClient) {
		client->sendMessage(":" + std::string(SERVER_NAME) + " 401 " + client->getNickname() + " " + targetNick + " :No such nick\r\n");
		return;
	}

	// Send WHOIS information
	// 311 RPL_WHOISUSER "<nick> <user> <host> * :<real name>"
	client->sendMessage(":" + std::string(SERVER_NAME) + " 311 " + client->getNickname() + " " +
		targetClient->getNickname() + " " + targetClient->getUsername() + " " +
		targetClient->getHostname() + " * :" + targetClient->getRealname() + "\r\n");

	// 312 RPL_WHOISSERVER "<nick> <server> :<server info>"
	client->sendMessage(":" + std::string(SERVER_NAME) + " 312 " + client->getNickname() + " " +
		targetClient->getNickname() + " " + std::string(SERVER_NAME) + " :ft_irc Server\r\n");

	// 318 RPL_ENDOFWHOIS "<nick> :End of WHOIS list"
	client->sendMessage(":" + std::string(SERVER_NAME) + " 318 " + client->getNickname() + " " +
		targetClient->getNickname() + " :End of WHOIS list\r\n");
}

bool ChannelsClientsManager::isNickInUse(const std::string& nickname) const {
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); it++)
	{
		if (it->second && it->second->getNickname() == nickname)
			return true;
	}
	return false;
}

void ChannelsClientsManager::registerClient(Client* client, IRCCommand& command) {
	if (command.getCommand() == "PASS") {
		if (command.getParams().at(0) == _password) {
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
	if (client->isAuthenticated() && client->isNicknameSet() && client->isUsernameSet()) {
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
	std::string keys = "";
	if (params.size() > 1)
		keys = params[1];
	size_t start = 0;
	size_t end = channels.find(',');
	size_t key_start = 0;
	size_t key_end = keys.find(',');
	while (start < channels.length())
	{
		std::string target;
		if (end == std::string::npos)
			target = channels.substr(start);
		else
			target = channels.substr(start, end - start);
		std::string key = "";
		if (!keys.empty() && key_start < keys.length()) {
			if (key_end == std::string::npos)
				key = keys.substr(key_start);
			else
				key = keys.substr(key_start, key_end - key_start);
		}
		if ((target[0] != '#' && target[0] != '&') || target.length() < 2)
		{
			client->sendMessage("server 403: sent invalid characters for channel name to JOIN\r\n");
			continueLoopJoin(start, end, channels);
			if (!keys.empty())
				continueLoopJoin(key_start, key_end, keys);
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
			if (!keys.empty())
				continueLoopJoin(key_start, key_end, keys);
			continue;
		}
		if (!the_first_one)
		{
			if (channel->isInviteOnly())
			{
				client->sendMessage("server 473: Can't join a channel (+i)\r\n");
				continueLoopJoin(start, end, channels);
				if (!keys.empty())
					continueLoopJoin(key_start, key_end, keys);
				continue;
			}
			if (!channel->getKey().empty())
			{
				if (key.empty() || key != channel->getKey())
				{
					client->sendMessage("server 475: Can't join a channel (wrong key (+k))\r\n");
					continueLoopJoin(start, end, channels);
					if (!keys.empty())
						continueLoopJoin(key_start, key_end, keys);
					continue;
				}
			}
			if (channel->getUserLimit() > 0 && channel->getClients().size() >= channel->getUserLimit())
			{
				client->sendMessage("server 471: can't join a full channel(+l)\r\n");
				continueLoopJoin(start, end, channels);
				if (!keys.empty())
					continueLoopJoin(key_start, key_end, keys);
				continue;
			}
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
		if (!keys.empty())
			continueLoopJoin(key_start, key_end, keys);
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
	if (command.getParamsCount() == 1)
	{
		if (!channel->getTopic().empty())
			client->sendMessage(channel->getTopic() + "\r\n");
		else
			client->sendMessage("Topic of this channel is not set yet\r\n");
		return;
	}
	std::string new_topic = params[1];
	if (channel->isTopicProtected() && !channel->isOperator(client))
	{
		client->sendMessage("server 482: You're not channel operator (topic is protected)(+t)\r\n");
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
	if (command.getParamsCount() >= 3)
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

	if (!client) // caller should provide a valid client for error replies
        return NULL;
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); it++)
    {
		Client* c = it->second;
		if (!c) // skip null pointers
            continue;
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

void ChannelsClientsManager::Key_check()
{
	
}


Channel* ChannelsClientsManager::getChannel(std::string const &channelName) {
	std::map<std::string, Channel*>::iterator it = _channels.find(channelName);
	if (it != _channels.end())
		return it->second;
	return NULL;
}

void ChannelsClientsManager::removeClient(Client &client) {
	// Remove client from all channels
	std::vector<std::string>& clientChannels = client.getChannels();
	for (size_t i = 0; i < clientChannels.size(); ++i) {
		std::string channelName = clientChannels[i];
		Channel* channel = getChannel(channelName);
		if (channel) {
			channel->removeClient(&client);
			// If the channel is empty after removal, delete it
			if (channel->getClients().empty()) {
				delete channel;
				_channels.erase(channelName);
			}
		}
	}
	// Remove client from the clients map
	_clients.erase(client.getFd());
	// Remove client's pollfd entry
	for (std::vector<pollfd>::iterator it = _pollfds.begin(); it != _pollfds.end(); ++it) {
		if (it->fd == client.getFd()) {
			_pollfds.erase(it);
			break;
		}
	}
	// Close the client's socket
	close(client.getFd());
	// Finally, delete the client object
	delete &client;
}


void ChannelsClientsManager::sendPingToClient(Client* client) {
	std::string pingMessage = "PING " + std::string(SERVER_NAME) + "\r\n";
	client->sendMessage(pingMessage);
}
