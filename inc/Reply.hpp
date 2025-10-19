#ifndef REPLY_HPP
#define REPLY_HPP


#include <string>
#include "ReplyNumbers.hpp"
#include "Client.hpp"
#include "ReplyNumbers.hpp"

class Reply {
private:
	static std::string build(const std::string& code, const std::string& target, const std::string& message);
public:
	static void welcome(const Client& client);
	static void passwordMismatch(const Client& client);
	static void alreadyRegistered(const Client& client);
	static void unknownCommand(const Client& client, const std::string& command);
	static void needMoreParams(const Client& client, const std::string& command);
	static void nicknameInUse(const Client& client, const std::string& nickname);
	static std::string noSuchNick(const std::string& target, const Client& client);
	static void connectionClosed(const Client& client);
	static void pongReply(const Client& client, const std::string& server);
	static void pingToClient(const Client& client, const std::string& server);
	static void noSuchChannel(const Client& client, const std::string& channel);
	static void notOnChannel(const Client& client, const std::string& channel);
	static void usersDontMatch(const Client& client);
	static void notOperator(const Client& client, const std::string& channel);
	static void invalidCommand(const Client& client, const std::string& command);
	static void messageTooLong(const Client& client);
};

#endif // REPLY_HPP
