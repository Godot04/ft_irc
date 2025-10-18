#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <algorithm>

#include <IRCCommand.hpp>
#include <ChannelsClientsManager.hpp>

# define PRINT_CLIENT_INFO 0
# define SEND_PING_AT_HALF_TIME 0
class Client;
class Channel;

class Server {
private:
    int                             _socket;
    int                             _port;
    std::string                     _password;
    struct sockaddr_in              _address;
    std::vector<pollfd>             _pollfds;  // pfd | plfd | plfd ....xpfd   (push_back(xpfd))
    std::map<int, Client*>          _clients;    // key - value pair.  key should be unique. 12 - popov 13 - khojazo   (_clients.at(12) - returns popov
    time_t                          _clientTimeToLive; // in seconds
    ChannelsClientsManager          _manager;
public:
    Server(int port, const std::string& password, time_t clientTimeToLive);
    ~Server();
    void    start();
    void    handleNewConnection();
    void    handleClientMessage(int clientfd);
};

#endif
