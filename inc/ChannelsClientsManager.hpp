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



// Yes, in the IRC protocol, if a client’s connection is not maintained (for example, due to network timeout, client disconnect, or socket error), the server removes the client from all channels and from the server itself.

// What happens:

// The client is removed from all channel member lists.
// The client is removed from all operator lists (if applicable).
// Other users in the same channels are notified with a QUIT message.


class ChannelsClientsManager {
public:
    ChannelsClientsManager();
    ~ChannelsClientsManager();
	void setClientsMap(std::map<int, Client*> *clients, std::string const *password, std::vector<pollfd> *pollfds);
	void handleClientMessage(Client* client);
    // void addClientToChannel(Client* client, Channel* channel);
    // void removeClientFromChannel(Client* client, Channel* channel);
    // std::vector<Client*> getClientsInChannel(Channel* channel);

private:
    std::map<std::string, Channel*>	_channels;
	std::map<int, Client*>			*_clients;
	std::string const				*_password;
	std::vector<pollfd>     		*_pollfds;

	// void							setNick(Client* client, IRCCommand& command);
	bool							isNickInUse(const std::string& nickname) const;
	void							registerClient(Client* client, IRCCommand& command);
	void							handleRegisteredClientMessage(Client* client, IRCCommand& command);
};







#endif
