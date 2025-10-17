#include "../inc/IRCCommand.hpp"
#include "../inc/ReplyNumbers.hpp"
#include <sstream>
#include <iostream>

IRCCommand::IRCCommand(std::string const & inputStr) :
    _input(inputStr), _cmd(""), _prefix(""), _params(), _errorNum(NO_ERROR), _isValid(false), _hasPrefix(false)

{
	if (_input.find("\r\n") == std::string::npos) {
		_isValid = false;
        _errorNum = ERR_NEEDMOREPARAMS;
		_cmd = _input;
	}
	else {
		processCommand();
	}
}

IRCCommand::~IRCCommand() {}

bool IRCCommand::isValid() const {
	return _isValid;
}

void IRCCommand::processCommand() {
    std::istringstream iss(_input);
    std::string word;
    bool first = true;
    while (iss >> word) {
        trimCRLF(word);
        if (first && word[0] == ':') {
            _prefix = word.substr(1); // Remove leading ':'
            if (_prefix.empty()) {
                _errorNum = ERR_NEEDMOREPARAMS;
            }
            _hasPrefix = true;
            continue;
        }
        if (word.empty()) {
            _errorNum = ERR_NEEDMOREPARAMS;
            return ;
        }
        else if (first) {
            _cmd = word;
            first = false;
        }
        if (!first)
            handleParameters(iss);
    }
    if (_cmd.empty()) {
        _isValid = false;
        _errorNum = ERR_NEEDMOREPARAMS;
        return ;
    }
}

void IRCCommand::handleParameters(std::istringstream &iss) {
    if (_cmd == "MODE")
        handleModeCmd(iss);
    else if (_cmd == "PASS")
        handlePassCmd(iss);
    else if (_cmd == "NICK")
        handleNickCmd(iss);
    else if (_cmd == "USER")
        handleUserCmd(iss);
    else if (_cmd == "CAP")
        handleCapCmd(iss);
    else if (_cmd == "JOIN")
        handleJoinCmd(iss);
    else if (_cmd == "PRIVMSG")
        handlePrivmsgCmd(iss);
    else if (_cmd == "INVITE")
        handleInviteCmd(iss);
    else if (_cmd == "KICK")
        handleKickCmd(iss);
    else if (_cmd == "TOPIC")
        handleTopicCmd(iss);
    else if (_cmd == "PING" || _cmd == "PONG")
        handlePingCmd(iss);
}

void IRCCommand::handlePingCmd(std::istringstream &iss) {
    std::string server;
    iss >> server; // extract server parameter

    if (server.empty()) {
        _isValid = false;
        _errorNum = ERR_NEEDMOREPARAMS;
        return;
    }
    if (server[0] == ':' && server.length() <= 1) {
        _isValid = false;
        _errorNum = ERR_NEEDMOREPARAMS;
        _params.push_back(server);
    }
    else {
        trimCRLF(server);
        _params.push_back(server);
        _isValid = true;
    }
    // Check for extra parameters
    std::string extra;
    if (iss >> extra) {
        _isValid = false;
        _errorNum = ERR_NEEDMOREPARAMS;
        // Add the extra parameters to params for completeness
        trimCRLF(extra);
        _params.push_back(extra);
        while (iss >> extra) {
            trimCRLF(extra);
            _params.push_back(extra);
        }
    }
}

void IRCCommand::handleJoinCmd(std::istringstream &iss) {
    std::string word;
    while (iss >> word) {
        trimCRLF(word);
        if (!word.empty()) {
            _params.push_back(word);
        }
    }
    if (_params.size() < 1) {
        _isValid = false;
        _errorNum = ERR_NEEDMOREPARAMS;
    }
    else {
        _isValid = true;
    }
}

void IRCCommand::handleCapCmd(std::istringstream &iss) {
    std::string word;
    while (iss >> word) {
        trimCRLF(word);
        if (!word.empty()) {
            _params.push_back(word);
        }
    }
    if (_params.size() < 1) {
        _isValid = false;
        _errorNum = ERR_NEEDMOREPARAMS;
    }
    else {
        _isValid = true;
    }
}

void IRCCommand::handleUserCmd(std::istringstream &iss) {
    std::string word;
    std::string userName, hostName, serverName, realName;
    iss >> userName >> hostName >> serverName;

    // Check if we have all required parameters
    if (userName.empty() || hostName.empty() || serverName.empty()) {
        _isValid = false;
        _errorNum = ERR_NEEDMOREPARAMS;
    }

    // Add non-empty parameters
    if (!userName.empty())
        _params.push_back(userName);
    if (!hostName.empty())
        _params.push_back(hostName);
    if (!serverName.empty())
        _params.push_back(serverName);

    // Read the rest as realname
    iss >> word;
    realName = word;
    while (iss >> word) {
        realName += " " + word;
    }

    // Trim spaces and check realname
    size_t firstNonSpace = realName.find_first_not_of(" \t");
    if (firstNonSpace != std::string::npos)
        realName = realName.substr(firstNonSpace);

    // Check if realname is valid
    if (realName.empty() || realName[0] != ':' || realName.length() <= 1) {
        _isValid = false;
        _errorNum = ERR_NEEDMOREPARAMS;
        if (!realName.empty())
            _params.push_back(realName);
    } else {
        trimCRLF(realName);
        _params.push_back(realName);
        _isValid = true;
    }
}

void IRCCommand::handleNickCmd(std::istringstream &iss) {
    std::string word;
    while (iss >> word) {
        trimCRLF(word);
        if (!word.empty()) {
            _params.push_back(word);
        }
        if (_params.size() == 1 && _errorNum == NO_ERROR) {
            _isValid = true;
        }
    }
    if (_params.size() != 1) {
        _isValid = false;
        _errorNum = ERR_NEEDMOREPARAMS;
    }
}

void IRCCommand::handlePassCmd(std::istringstream &iss) {
    std::string word;
    while (iss >> word) {
        trimCRLF(word);
        if (!word.empty()) {
            _params.push_back(word);
            _isValid = true;
        }
    }
    if (_params.size() != 1) {
        _isValid = false;
        _errorNum = ERR_NEEDMOREPARAMS;
    }
}

void IRCCommand::handleModeCmd(std::istringstream &iss) {
    std::string word;
    std::string flags;

    while (iss >> word) {
        if (!word.empty()) {
            trimCRLF(word);
            _params.push_back(word);
        }
        else {
            return ;
        }

        if (_params.size() == 1 && word[0] != '#') {
            _errorNum = ERR_NEEDMOREPARAMS;
        }
        else if (_params.size() == 2 && !isFlagSetValid(word)) {
            _errorNum = ERR_NEEDMOREPARAMS;
        }
        else if (_params.size() == 2) {
            _modeFlags = word;
            handleModeCmdParams(iss);
        }
        if (!_errorNum.empty()) {
            _isValid = false;
        }
        else {
            _isValid = true;
        }
    }
}

void IRCCommand::handleModeCmdParams(std::istringstream &iss) {
    ModeFlag currentFlag;
    while ((currentFlag = getModeFlag()) != MODE_UNKNOWN) {
        if (currentFlag == MODE_KEY || currentFlag == MODE_KEY) {
            std::string param;
            if (iss >> param) {
                trimCRLF(param);
                _params.push_back(param);
            } else {
                _isValid = false;
                _errorNum = ERR_NEEDMOREPARAMS;
                return ;
            }
        }
        else if (currentFlag == MODE_LIMIT_USER) {
            std::string limitParam;
            if (iss >> limitParam) {
                trimCRLF(limitParam);
                // Check if limitParam is a valid number
                // find_first_not_of - returns first character that is not in the set of characters you provide
                if (limitParam.find_first_not_of("0123456789") != std::string::npos) {
                    _isValid = false;
                    _errorNum = ERR_NEEDMOREPARAMS;
                    _params.push_back(limitParam);
                    while (iss >> limitParam) {
                        trimCRLF(limitParam);
                        _params.push_back(limitParam);
                    }
                    return ;
                }
                _params.push_back(limitParam);
            } else {
                _isValid = false;
                _errorNum = ERR_NEEDMOREPARAMS;
                return ;
            }
        }
    }
}

ModeFlag IRCCommand::getModeFlag() {
    while (!_modeFlags.empty()) {
        if (_modeFlags.at(0) == '+') {
            _currentModeSign = PLUS;
            _modeFlags.erase(0, 1);
        }
        else if (_modeFlags.at(0) == '-') {
            _currentModeSign = MINUS;
            _modeFlags.erase(0, 1);
        }
        else {
            char flag = _modeFlags.at(0);
            _modeFlags.erase(0, 1);
            // return flag;
            switch (flag) {
                case 'i': return MODE_INVITE;
                case 't': return MODE_TOPIC;
                case 'k': return MODE_KEY;
                case 'o': return MODE_OPERATOR;
                case 'l': return MODE_LIMIT_USER;
            default:  return MODE_UNKNOWN;
            }
        }
    }
    return MODE_UNKNOWN;
}

ModeSign const &IRCCommand::getCurrentModeSign() const {
    return _currentModeSign;
}

std::string const &IRCCommand::getCommand() const {
	return _cmd;
}

std::vector<std::string> const &IRCCommand::getParams() const {
	return _params;
}

std::string const &IRCCommand::getPrefix() const {
    return _prefix;
}

std::string const &IRCCommand::getErrorNum() const {
    return _errorNum;
}

void IRCCommand::handlePrivmsgCmd(std::istringstream &iss) {
    std::string target;
    iss >> target; // extract channel or nickname

    if (target.empty()) {
        _isValid = false;
        _errorNum = ERR_NEEDMOREPARAMS;
        return;
    }

    _params.push_back(target);

    // Read the rest as message
    std::string message;
    std::getline(iss, message);

    // Trim leading spaces
    size_t firstNonSpace = message.find_first_not_of(" \t");
    if (firstNonSpace != std::string::npos)
        message = message.substr(firstNonSpace);

    // Check if message starts with ':'
    if (!message.empty() && message[0] == ':')
        message = message.substr(1);

    trimCRLF(message);

    if (message.empty()) {
        _isValid = false;
        _errorNum = ERR_NEEDMOREPARAMS;
        _params.push_back("");
        return;
    }

    _params.push_back(message);
    _isValid = true;
}

void IRCCommand::handleInviteCmd(std::istringstream &iss) {
    std::string target_nick;
    std::string target_channel;

    iss >> target_nick >> target_channel;
    trimCRLF(target_channel);

    if (target_nick.empty() || target_channel.empty()) {
        _isValid = false;
        _errorNum = ERR_NEEDMOREPARAMS;
        if (!target_nick.empty())
            _params.push_back(target_nick);
        if (!target_channel.empty())
            _params.push_back(target_channel);
        return;
    }

    _params.push_back(target_nick);
    _params.push_back(target_channel);
    _isValid = true;
}

void IRCCommand::handleKickCmd(std::istringstream &iss) {
    std::string target_channel;
    std::string target_nick;

    iss >> target_channel >> target_nick;

    trimCRLF(target_channel);
    trimCRLF(target_nick);

    if (target_channel.empty() || target_nick.empty()) {
        _isValid = false;
        _errorNum = ERR_NEEDMOREPARAMS;
        if (!target_channel.empty())
            _params.push_back(target_channel);
        if (!target_nick.empty())
            _params.push_back(target_nick);
        return;
    }

    _params.push_back(target_channel);
    _params.push_back(target_nick);

    // Read optional kick message
    std::string kick_message;
    std::getline(iss, kick_message);

    // Trim leading spaces
    size_t firstNonSpace = kick_message.find_first_not_of(" \t");
    if (firstNonSpace != std::string::npos)
        kick_message = kick_message.substr(firstNonSpace);

    // Check if message starts with ':'
    if (!kick_message.empty() && kick_message[0] == ':')
        kick_message = kick_message.substr(1);

    trimCRLF(kick_message);

    if (!kick_message.empty())
        _params.push_back(kick_message);

    _isValid = true;
}

void IRCCommand::handleTopicCmd(std::istringstream &iss) {
    std::string target_channel;
    iss >> target_channel;

    trimCRLF(target_channel);

    if (target_channel.empty()) {
        _isValid = false;
        _errorNum = ERR_NEEDMOREPARAMS;
        return;
    }

    _params.push_back(target_channel);

    // Read optional topic
    std::string new_topic;
    std::getline(iss, new_topic);

    // Trim leading spaces
    size_t firstNonSpace = new_topic.find_first_not_of(" \t");
    if (firstNonSpace != std::string::npos)
        new_topic = new_topic.substr(firstNonSpace);

    // Check if topic starts with ':'
    if (!new_topic.empty() && new_topic[0] == ':')
        new_topic = new_topic.substr(1);

    trimCRLF(new_topic);

    if (!new_topic.empty())
        _params.push_back(new_topic);

    _isValid = true;
}

void IRCCommand::trimCRLF(std::string &str) {
    // Remove \r\n at the end
    if (str.size() >= 2 && str.substr(str.size() - 2) == "\r\n") {
        str = str.substr(0, str.size() - 2);
    }
    // Remove \r at the end (in case only \r remains after getline)
    else if (str.size() >= 1 && str[str.size() - 1] == '\r') {
        str = str.substr(0, str.size() - 1);
    }
    // Remove \n at the end (in case only \n remains)
    else if (str.size() >= 1 && str[str.size() - 1] == '\n') {
        str = str.substr(0, str.size() - 1);
    }
}

bool IRCCommand::isFlagSetValid(std::string const &flags) const {
    char c;
    if (flags.size() >= 1) {
        c = flags[0];
        if (c != '+' && c != '-')
            return false;
    }
    for (size_t i = 0; i < flags.size(); ++i) {
        c = flags[i];
        if (c != 'i' && c != 't' && c != 'k' && c != 'o' && c != 'l' && c != '+' && c != '-') {
            return false;
        }
    }
    return true;
}

std::string IRCCommand::getParamAt(size_t index) const {
    if (index < _params.size()) {
        return _params[index];
    }
    return "";
}

size_t IRCCommand::getParamsCount() const {
    return _params.size();
}
