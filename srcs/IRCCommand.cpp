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

void IRCCommand::trimCRLF(std::string &str) {
    if (str.size() >= 2 && str.substr(str.size() - 2) == "\r\n") {
        str = str.substr(0, str.size() - 2);
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

//    Command: PASS
//    Parameters: <password>

