#include "../inc/ft_irc.hpp"

Client::Client(int fd) 
    : _fd(fd), _authenticated(false), _registered(false)
{
}

Client::~Client()
{
}

void Client::addToBuffer(const std::string& msg)
{
    _buffer += msg;
}

bool Client::hasCompleteMessage() const
{
    return _buffer.find("\\r\\n") != std::string::npos;
}

std::string Client::getNextMessage()
{
    size_t pos = _buffer.find("\\r\\n");
    if (pos == std::string::npos)
        return "";
    
    std::string message = _buffer.substr(0, pos);
    _buffer = _buffer.substr(pos + 2);
    return message;
}

void Client::clearBuffer()
{
    _buffer.clear();
}

void Client::sendMessage(const std::string& msg) const
{
    send(_fd, msg.c_str(), msg.length(), 0);
}

int Client::getFd() const
{
    return _fd;
}

const std::string& Client::getNickname() const
{
    return _nickname;
}

void Client::setNickname(const std::string& nickname)
{
    _nickname = nickname;
}

const std::string& Client::getUsername() const
{
    return _username;
}

void Client::setUsername(const std::string& username)
{
    _username = username;
}

const std::string& Client::getRealname() const
{
    return _realname;
}

void Client::setRealname(const std::string& realname)
{
    _realname = realname;
}

const std::string& Client::getHostname() const
{
    return _hostname;
}

void Client::setHostname(const std::string& hostname)
{
    _hostname = hostname;
}

bool Client::isAuthenticated() const
{
    return _authenticated;
}

void Client::setAuthenticated(bool authenticated)
{
    _authenticated = authenticated;
}

bool Client::isRegistered() const
{
    return _registered;
}

void Client::setRegistered(bool registered)
{
    _registered = registered;
}

std::vector<std::string>& Client::getChannels()
{
    return _channels;
}