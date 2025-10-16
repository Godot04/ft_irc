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
};

#endif // REPLY_HPP