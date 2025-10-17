#ifndef IRCCOMMAND_HPP
#define IRCCOMMAND_HPP

#include <string>
#include <vector>
#include <map>
#include "Channel.hpp"
#include "Client.hpp"

enum ModeSign {
	NONE,
	PLUS,
	MINUS
};
enum ModeFlag {
	MODE_UNKNOWN,
	MODE_INVITE, // invite-only
	MODE_TOPIC, // topic protection
	MODE_KEY, // key (password)
	MODE_OPERATOR, // operator
	MODE_LIMIT_USER // user limit
};

class IRCCommand {
public:
    static ModeFlag charToModeFlag(char c) {
        switch (c) {
            case 'i': return MODE_INVITE;
            case 't': return MODE_TOPIC;
            case 'k': return MODE_KEY;
            case 'o': return MODE_OPERATOR;
            case 'l': return MODE_LIMIT_USER;
		default:  return MODE_UNKNOWN;
        }
    }
private:
	std::string _input;
	std::string _cmd;
	std::string _prefix;
	std::vector<std::string> _params;
	std::string _errorNum;
	ModeSign _currentModeSign;
	bool _isValid;
	std::string _modeFlags;
	bool _hasPrefix;

	void							handleParameters(std::istringstream &iss);
	void							handleModeCmd(std::istringstream &iss);
	void							handleModeCmdParams(std::istringstream &iss);
	void							handlePassCmd(std::istringstream &iss);
	void							handleNickCmd(std::istringstream &iss);
	void							handleUserCmd(std::istringstream &iss);
	void							handleCapCmd(std::istringstream &iss);
	void							handleJoinCmd(std::istringstream &iss);
	void							handlePrivmsgCmd(std::istringstream &iss);
	void							handleInviteCmd(std::istringstream &iss);
	void							handleKickCmd(std::istringstream &iss);
	void							handleTopicCmd(std::istringstream &iss);
	void							handlePingCmd(std::istringstream &iss);
	void							processCommand();
	void							trimCRLF(std::string &str);
public:
	IRCCommand(std::string const & command);
	~IRCCommand();
	bool							isValid()const;
	std::string const				&getCommand() const;
	std::string const				&getPrefix() const;
	std::vector<std::string> const	&getParams() const;
	// getParamAt(2)
	std::string const				&getErrorNum() const;
	ModeSign const					&getCurrentModeSign() const;
	ModeFlag						getModeFlag();
	bool							isFlagSetValid(std::string const &flags) const;

};

#endif
