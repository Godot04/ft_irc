#include "../inc/ft_irc.hpp"

Client::Client(int fd)
    : _fd(fd), _authenticated(false), _registered(false), _isCAPNegotiation(false), _connectionTime(time(NULL))
{
}

Client::~Client()
{
}

void Client::addToBuffer(const std::string& msg)
{
    _buffer += msg;
    if (_buffer.size() > 2048) {
        Reply::messageTooLong(*this);
        _buffer.clear();
    }
}

bool Client::hasCompleteMessage() const
{
    return _buffer.find("\r\n") != std::string::npos;
}

std::string Client::getNextMessage()
{
    size_t pos = _buffer.find("\r\n");
    if (pos == std::string::npos)
        return "";

    std::string message = _buffer.substr(0, pos + 2);
    _buffer = _buffer.substr(pos + 2);
    return message;
}

void Client::clearBuffer()
{
    _buffer.clear();
}

void Client::printBuffer() const
{
    std::cout << "Client Buffer: " << _buffer << std::endl;
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

void Client::addChannel(const std::string& channel)
{
    for (std::vector<std::string>::iterator it = _channels.begin(); it != _channels.end(); ++it)
    {
        if (*it == channel)
            return; // Channel already in list
    }
    _channels.push_back(channel);
}

void Client::removeChannel(const std::string& channel)
{
    std::vector <std::string>::iterator it = std::find(_channels.begin(), _channels.end(), channel);
    if (it != _channels.end())
        _channels.erase(it);
}

std::string const & Client::getBuffer() const
{
    return _buffer;
}

bool Client::isCAPNegotiation() const
{
    return _isCAPNegotiation;
}

void Client::setCAPNegotiation(bool status)
{
    _isCAPNegotiation = status;
}

void Client::updateConnectionTime() {
    _connectionTime = time(NULL); // in seconds
}

time_t Client::getTimePassed() const {
    return time(NULL) - _connectionTime;
}

void Client::printClientInfo() const
{
    std::cout << "Client FD: " << _fd << std::endl;
    std::cout << "Nickname: " << _nickname << std::endl;
    std::cout << "Username: " << _username << std::endl;
    std::cout << "Realname: " << _realname << std::endl;
    std::cout << "Hostname: " << _hostname << std::endl;
    std::cout << "Authenticated: " << (_authenticated ? "Yes" : "No") << std::endl;
    std::cout << "Registered: " << (_registered ? "Yes" : "No") << std::endl;
    std::cout << "Channels: ";
    for (std::vector<std::string>::const_iterator it = _channels.begin(); it != _channels.end(); ++it)
        std::cout << *it << " ";
    std::cout << std::endl;
}
