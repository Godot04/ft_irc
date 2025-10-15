#include "../inc/ft_irc.hpp"
#include "Reply.hpp"

#define KICK  "KICK"


bool Server::handleOperatorCommand(Client* client, std::istringstream& iss) {
	if (processKick(client, iss)) {
		return true;
	}
	return false;
}


// Command: KICK
//    Parameters: <channel> <user> [<comment>]
// e.g.
// KICK &Melbourne Matthew         ; Kick Matthew from &Melbourne

// KICK #Finnish John :Speaking English
//                                 ; Kick John from #Finnish using
//                                 "Speaking English" as the reason
//                                 (comment).

// :WiZ KICK #Finnish John         ; KICK message from WiZ to remove John
//                                 from channel #Finnish


bool Server::processKick(Client* client, std::istringstream& iss) {
	std::string first, second, third, fourth;
	iss >> first >> second >> third;

	if (first.empty() || second.empty() || third.empty()) {
		Reply::needMoreParams(*client, KICK);
		return false;
	}
	if (first == KICK) {
		if (second.at(0) == '&') {  // is cliebnt operator, is the other client in the channel .. delete
			second = second.substr(1);
			if (_channels.find(second) == _channels.end()) {
				client->sendMessage("No such channel\r\n");
			}
			Channel* channel = _channels.at(second);
			if (!channel->isOperator(client)) {
				client->sendMessage("You are not an operator of this channel\r\n");
			}
			if (channel->getClients().size() <= 1) {
				client->sendMessage("You can't kick yourself\r\n");
			}
			if (!channel->isClientInChannel(client)) {
				client->sendMessage("You are not in this channel\r\n");
			}
			// Check if the user to be kicked is in the channel
			if (!channel->isClientInChannel(third)) {
				client->sendMessage("User is not in this channel\r\n");
			}
			// user in channel, kick them out
			channel->removeClient(third);
			return true;
		}
		// # is cliebnt operator, is the other client in the channel .. delete
		// 
	}
	return false;
}



// command class
// command 
