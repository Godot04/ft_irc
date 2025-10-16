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
	{
		Reply::alreadyRegistered(*client);
	}
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
	else if (command.getCommand() == "JOIN") {
		// Handle JOIN command
	}
	else if (command.getCommand() == "PART") {
		// Handle PART command
	}
	else if (command.getCommand() == "PRIVMSG") {
	}
	else if (command.getCommand() == "QUIT") {
		// Handle QUIT command
	}
	else {
		Reply::unknownCommand(*client, command.getCommand());
	}
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
