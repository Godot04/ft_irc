
#include "Reply.hpp"
#include "Client.hpp"
#include <sstream>
#include <string>

// Reply::Reply() {}

// Reply::~Reply() {}

std::string Reply::build(const std::string& code, const std::string& target, const std::string& message) {
    std::ostringstream oss;
    oss << ":" << SERVER_NAME << " " << code << " " << target << " :" << message << "\r\n";
    return oss.str();
}

// Example reply builders (add more as needed)
void Reply::welcome(const Client& client) {
    client.sendMessage(Reply::build(RPL_WELCOME, client.getNickname(), "Welcome to the ft_IRC Network"));
}

void Reply::passwordMismatch(const Client& client) {
    client.sendMessage(build(ERR_PASSWDMISMATCH, "*", "Password incorrect. Usage: PASS <password>"));
}

void Reply::alreadyRegistered(const Client& client) {
    client.sendMessage(build(ERR_ALREADYREGISTRED, client.getNickname(), "You may not reregister"));
}

void Reply::unknownCommand(const Client& client, const std::string& command) {
    client.sendMessage(build(ERR_UNKNOWNCOMMAND, client.getNickname(), command + " Unknown command"));
}

void Reply::needMoreParams(const Client& client, const std::string& command) {
    client.sendMessage(build(ERR_NEEDMOREPARAMS, client.getNickname(), command + " Not enough parameters"));
}

void Reply::nicknameInUse(const Client& client, const std::string& nickname) {
    client.sendMessage(build(ERR_NICKNAMEINUSE, "*", nickname + " Nickname is already in use"));
}

std::string Reply::noSuchNick(const std::string& target, const Client& client) {
    return build(ERR_NOSUCHNICK, target, client.getNickname() + " No such nick/channel");
}

void Reply::connectionClosed(const Client& client) {
    client.sendMessage("Connection closed\r\n");
}

void Reply::pongReply(const Client& client, const std::string& server) {
    client.sendMessage("PONG " + server + "\r\n");
}

void Reply::pingToClient(const Client& client, const std::string& server) {
    client.sendMessage("PING :" + server + "\r\n");
}

void Reply::noSuchChannel(const Client& client, const std::string& channel) {
    client.sendMessage(build(ERR_NOSUCHCHANNEL, channel, " No such channel"));
}

void Reply::notOnChannel(const Client& client, const std::string& channel) {
    client.sendMessage(build(ERR_NOTONCHANNEL, channel, " You're not on that channel"));
}

void Reply::usersDontMatch(const Client& client) {
    client.sendMessage(build(ERR_USERSDONTMATCH, client.getNickname(), "Cannot change mode for other users"));
}

void Reply::notOperator(const Client& client, const std::string& channel) {
    client.sendMessage(build(ERR_CHANOPRIVSNEEDED, channel, "You're not channel operator"));
}

void Reply::invalidCommand(const Client& client, const std::string& command) {
    client.sendMessage(build(ERR_UNKNOWNCOMMAND, client.getNickname(), command + " :Invalid command format"));
}
