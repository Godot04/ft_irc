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

class Client;
class Channel;

class Server {
private:
    int                     _socket;
    int                     _port;
    std::string             _password;
    struct sockaddr_in      _address;
    std::vector<pollfd>     _pollfds;  // pfd | plfd | plfd ....xpfd   (push_back(xpfd))
    std::map<int, Client*>  _clients;    // key - value pair.  key should be unique. 12 - popov 13 - khojazo   (_clients.at(12) - returns popov
    std::map<std::string, Channel*> _channels; // next step is to make it possible for clients to create channels

public:
    Server(int port, const std::string& password);
    ~Server();

    void    start();
    void    handleNewConnection();
    void    handleClientMessage(int clientfd);
    void    handleClientCommands(Client *client, std::istringstream &iss);
    void    removeClient(int clientfd);
    void    registerClient(Client* client, std::istringstream& iss);
    void	processPassword(Client* client, std::istringstream& iss);
	void	processNick(Client* client, std::istringstream& iss);
	void	processUser(Client* client, std::istringstream& iss);
    void    processPrivmsg(Client* client, std::istringstream& iss);
    void    processJoin(Client* client, std::istringstream& iss);
    void    handleCAP(Client* client, std::istringstream& iss);
    // Getters
    std::map<int, Client*>&              getClients();
    std::map<std::string, Channel*>&     getChannels();
    const std::string&                   getPassword() const;
};

#endif
