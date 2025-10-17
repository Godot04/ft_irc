#ifndef CHANNELSCLIENTSMANAGER_HPP
#define CHANNELSCLIENTSMANAGER_HPP

#include "Channel.hpp"
#include "Client.hpp"
#include "IRCCommand.hpp"
#include "Reply.hpp"
#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <poll.h>
#include <unistd.h>
#include <cstdlib>


// Yes, in the IRC protocol, if a client’s connection is not maintained (for example, due to network timeout, client disconnect, or socket error), the server removes the client from all channels and from the server itself.

// What happens:

// The client is removed from all channel member lists.
// The client is removed from all operator lists (if applicable).
// Other users in the same channels are notified with a QUIT message.


class ChannelsClientsManager {
public:
    ChannelsClientsManager(std::map<int, Client*> &clients, std::string const &password, std::vector<pollfd> &pollfds);
    ~ChannelsClientsManager();
	void setClientsMap(std::map<int, Client*> *clients, std::string const *password, std::vector<pollfd> *pollfds);
	void handleClientMessage(Client* client);
    // void addClientToChannel(Client* client, Channel* channel);
    // void removeClientFromChannel(Client* client, Channel* channel);
    // std::vector<Client*> getClientsInChannel(Channel* channel);
	Channel							*getChannel(std::string const &channelName); // if null, channel doesn't exit
	int								getPollSize() const { return _pollfds.size(); }
	int								getClientsSize() const { return _clients.size(); }
	int								getChannelsSize() const { return _channels.size(); }
	void							removeClient(Client &client);
	void							sendPingToClient(Client* client);
private:
    std::map<std::string, Channel*>	_channels;
	std::map<int, Client*>			&_clients;
	std::string const				&_password;
	std::vector<pollfd>     		&_pollfds;

	// void							setNick(Client* client, IRCCommand& command);
	// Registration
	bool							isNickInUse(const std::string& nickname) const;
	void							registerClient(Client* client, IRCCommand& command);
	void							handleRegisteredClientMessage(Client* client, IRCCommand& command);
	// Command execution
	void							executePrivmsg(Client* client, IRCCommand& command);
	void							executeJoin(Client* client, IRCCommand& command);
	void							executeInvite(Client* client, IRCCommand& command);
	void							executeTopic(Client* client, IRCCommand& command);
	void							executeKick(Client* client, IRCCommand& command);
	void							executePing(Client* client, IRCCommand& command);
	void							executeMode(Client* client, IRCCommand& command);
	// Helper functions
	void							handleModeFlags(Client &client, Channel &channel, IRCCommand& command);
	Client*							getClientByNickname(const std::string& nickname, Client* client);
	void							continueLoopJoin(size_t &start, size_t &end, const std::string& channels);
};







#endif
