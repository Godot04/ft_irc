#ifndef FT_IRC_HPP
#define FT_IRC_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <sstream>

#define BUFFER_SIZE 512

class Client;
class Channel;

class Server {
private:
    int                     _socket;
    int                     _port;
    std::string             _password;
    struct sockaddr_in      _address;
    std::vector<pollfd>     _pollfds;
    std::map<int, Client*>  _clients;
    std::map<std::string, Channel*> _channels;

public:
    Server(int port, const std::string& password);
    ~Server();

    void    start();
    void    handleNewConnection();
    void    handleClientMessage(int clientfd);
    void    removeClient(int clientfd);
    
    // Getters
    std::map<int, Client*>&              getClients();
    std::map<std::string, Channel*>&     getChannels();
    const std::string&                   getPassword() const;
};

class Client {
private:
    int                 _fd;
    std::string         _nickname;
    std::string         _username;
    std::string         _realname;
    std::string         _hostname;
    bool                _authenticated;
    bool                _registered;
    std::vector<std::string> _channels;
    std::string         _buffer;

public:
    Client(int fd);
    ~Client();

    void                addToBuffer(const std::string& msg);
    bool                hasCompleteMessage() const;
    std::string         getNextMessage();
    void                clearBuffer();
    void                sendMessage(const std::string& msg) const;
    
    // Getters & Setters
    int                 getFd() const;
    const std::string&  getNickname() const;
    void                setNickname(const std::string& nickname);
    const std::string&  getUsername() const;
    void                setUsername(const std::string& username);
    const std::string&  getRealname() const;
    void                setRealname(const std::string& realname);
    const std::string&  getHostname() const;
    void                setHostname(const std::string& hostname);
    bool                isAuthenticated() const;
    void                setAuthenticated(bool authenticated);
    bool                isRegistered() const;
    void                setRegistered(bool registered);
    std::vector<std::string>& getChannels();
};

class Channel {
private:
    std::string                 _name;
    std::string                 _topic;
    std::vector<Client*>        _clients;
    std::vector<Client*>        _operators;

public:
    Channel(const std::string& name);
    ~Channel();

    void                addClient(Client* client);
    void                removeClient(Client* client);
    void                addOperator(Client* client);
    void                removeOperator(Client* client);
    void                broadcast(const std::string& message, Client* sender = NULL);
    bool                isClientInChannel(Client* client) const;
    bool                isOperator(Client* client) const;
    
    // Getters & Setters
    const std::string&  getName() const;
    const std::string&  getTopic() const;
    void                setTopic(const std::string& topic);
    std::vector<Client*>& getClients();
    std::vector<Client*>& getOperators();
};

class Command {
protected:
    Server*     _server;
    Client*     _client;
    std::vector<std::string> _params;
    
public:
    Command(Server* server, Client* client, const std::vector<std::string>& params);
    virtual ~Command();
    virtual void execute() = 0;
};

#endif