#include "../inc/ft_irc.hpp"
#include "Reply.hpp"

Server::Server(int port, const std::string& password)
    : _port(port), _password(password)
{
    // Create socket
    // AF_INET      IPv4 Internet protocols
    // SOCK_STREAM is for TCP
    // zero - use the default protocol for this address family + type” (for AF_INET + SOCK_STREAM that's IPPROTO_TCP or zero).
    _socket = socket(AF_INET, SOCK_STREAM, 0);
    if (_socket < 0)
        throw std::runtime_error("Failed to create socket");
    // Set socket options
	// SOL_SOCKET to manipulate options at the sockets API level
	// SO_REUSEADDR to allow reuse of local addresses address:port.
    // When a socket is closed, that address:port pair is put into a TIME_WAIT state,
    // typically for a few minutes. During this time, the address:port pair cannot be reused.
    // Setting this option allows a socket to forcibly bind to a port in use by a socket in TIME_WAIT state.
	// opt = 1 to enable the option
    // SOL_SOCKET applies this option at the generic socket API level (not protocol-specific)
    // opt = 1 means “enable” the boolean socket option (SO_REUSEADDR).
    // The size argument tells the kernel how many bytes to read from the pointer you pass to setsockopt/getsockopt — it must match the option's expected type.
    int opt = 1;
    if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        close(_socket);
        throw std::runtime_error("Failed to set socket options");
    }

    // Set non-blocking mode
    // F_SETFL tells fcntl to set the file status flags to the value given by the third argument.
    // O_NONBLOCK is a flag that makes I/O on the descriptor non‑blocking: calls like accept(),
    // recv(), send() return immediately. If no data/connection is available they return -1
    // and set errno to EAGAIN or EWOULDBLOCK instead of waiting.
    if (fcntl(_socket, F_SETFL, O_NONBLOCK) < 0)
    {
        close(_socket);
        throw std::runtime_error("Failed to set non-blocking mode");
    }

    // Initialize address structure
    memset(&_address, 0, sizeof(_address));
    _address.sin_family = AF_INET;
    _address.sin_addr.s_addr = INADDR_ANY;
    _address.sin_port = htons(_port); // converts a 16-bit unsigned integer from host byte order to network byte order

    // Bind socket to address
    if (bind(_socket, (struct sockaddr *)&_address, sizeof(_address)) < 0)
    {
        close(_socket);
        throw std::runtime_error("Failed to bind socket");
    }

    // Listen for connections
    // listen(fd, backlog) marks a previously created and bound TCP socket as passive:
    // the kernel will start accepting incoming TCP connection attempts on that socket
    // and queue them until your program calls accept().
    if (listen(_socket, 10) < 0)
    {
        close(_socket);
        throw std::runtime_error("Failed to listen on socket");
    }

    // Add server socket to pollfds
    pollfd pfd;
    pfd.fd = _socket;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _pollfds.push_back(pfd);

    std::cout << "Server initialized on port " << _port << std::endl;
}

Server::~Server()
{
    // Close all client connections
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        close(it->first);
        delete it->second;
    }

    // Delete all channels
    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
        delete it->second;

    // Close server socket
    close(_socket);
    std::cout << "Server shut down" << std::endl;
}

void Server::start()
{
    std::cout << "Server started. Waiting for connections..." << std::endl;

    while (true)
    {
        if (poll(&_pollfds[0], _pollfds.size(), -1) < 0)
        {
            if (errno == EINTR)
                continue;
            throw std::runtime_error("Poll failed");
        }
        // Check for activity on each socket
        for (size_t i = 0; i < _pollfds.size(); ++i)
        {
            if (_pollfds[i].revents & POLLIN)
            {
                // if new connection then the fd
                if (_pollfds[i].fd == _socket) {
					std::cout << "New connection attempt detected." << std::endl;
                    handleNewConnection();
				}
                else {
					std::cout << "Data received from client (fd: " << _pollfds[i].fd << ")" << std::endl;
                    handleClientMessage(_pollfds[i].fd);
				}
            }
            else if (_pollfds[i].revents & (POLLHUP | POLLERR))
            {
                if (_pollfds[i].fd != _socket)
                    removeClient(_pollfds[i].fd);
            }
        }
    }
}

void Server::handleNewConnection()
{
    struct sockaddr_in client_addr;
    socklen_t addr_size = sizeof(client_addr);
    //    The accept() system call is used with connection-based socket
    //    types (SOCK_STREAM, SOCK_SEQPACKET).  It extracts the first
    //    connection request on the queue of pending connections for the
    //    listening socket, sockfd, creates a new connected socket, and
    //    returns a new file descriptor referring to that socket.
    int client_fd = accept(_socket, (struct sockaddr *)&client_addr, &addr_size);
    if (client_fd < 0)
    {
        std::cerr << "Failed to accept connection: " << strerror(errno) << std::endl;
        return;
    }

    // Set non-blocking mode
    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0)
    {
        std::cerr << "Failed to set non-blocking mode for client: " << strerror(errno) << std::endl;
        close(client_fd);
        return;
    }

    // Add to pollfds
    pollfd pfd;
    pfd.fd = client_fd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _pollfds.push_back(pfd);

    // Create client
    Client *client = new Client(client_fd);
    _clients[client_fd] = client;
    // _clients[client_fd] = new Client(client_fd);

    // Set hostname
    // inet_ntoa(client_addr.sin_addr) converts the IPv4 address in
    // client_addr.sin_addr into a human‑readable dotted string like "192.168.0.5".
    client->setHostname(inet_ntoa(client_addr.sin_addr));

    std::cout << "New connection from " << client->getHostname() << " (fd: " << client_fd << ")" << std::endl;

    Reply::welcome(*client);
}

// bla-bla
void Server::handleClientMessage(int clientfd)
{
    // Recieve message
    Client *client = _clients[clientfd];
    char buffer[BUFFER_SIZE + 1];
    memset(buffer, 0, sizeof(buffer));

    ssize_t bytes_read = recv(clientfd, buffer, BUFFER_SIZE, 0);
    if (bytes_read <= 0)
    {
        if (bytes_read == 0 || errno != EAGAIN)
            removeClient(clientfd);
        return;
    }
    client->addToBuffer(std::string(buffer, bytes_read));
    if (!client->hasCompleteMessage())
    {
        return ; // Wait for more data
    }
    for (std::string message = client->getNextMessage(); !message.empty(); message = client->getNextMessage())
    {
        std::istringstream iss(message);
        registerClient(client, iss);
        if (client->isRegistered())
        {
            std::istringstream cmdIss(message);  // reset to start
            handleClientCommands(client, cmdIss);
            if (client->isRegistered() &&
                std::find(client->getChannels().begin(), client->getChannels().end(), "#general") == client->getChannels().end())
            {
                bool the_first_one = false;
                std::string defaultChannel = "#general";
                Channel* channel;
                if (_channels.find(defaultChannel) == _channels.end())
                {
                    channel = new Channel(defaultChannel);
                    _channels[defaultChannel] = channel;
                    the_first_one = true;
                    channel->setTopic("Channel for old fellows\r\n");
                }
                else
                    channel = _channels[defaultChannel];
                if (the_first_one)
                    channel->addOperator(client);
                else
                    channel->addClient(client);
                client->addChannel(defaultChannel);
                std::string join_msg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + " JOIN :" + defaultChannel + "\r\n";
                client->sendMessage(join_msg);
                channel->broadcast(join_msg, client);
                client->sendMessage(channel->getTopic() + "\r\n");
            }
        }
    }
    client->clearBuffer();
    if (PRINT_CLIENT_INFO && client->isRegistered())
        client->printClientInfo();
}

void Server::registerClient(Client* client, std::istringstream& iss) {
    std::string command;
    iss >> command;
    if (command == "CAP") {
        handleCAP(client, iss);
    }
    if (command == "PASS") {
        processPassword(client, iss);
    }
    else if (!client->isAuthenticated())
        Reply::passwordMismatch(*client);
    else if (command == "NICK")
        processNick(client, iss);
    else if (command == "USER")
        processUser(client, iss);
    else if (!client->isRegistered())
        Reply::unknownCommand(*client, command);
    if (!client->isRegistered() && client->isAuthenticated() && client->isNicknameSet() &&
        client->isUsernameSet() && !client->isCAPNegotiation())
    {
        client->setRegistered(true);
    }
}

void Server::handleClientCommands(Client *client, std::istringstream &iss)
{
    std::string command;
    iss >> command;
    if (command == "PRIVMSG")
        processPrivmsg(client, iss);
    else if (command == "JOIN")
        processJoin(client, iss);
    // else if (command == "INVITE")
    //     processInvite(client, iss);
    else if (command == "TOPIC")
        processTopic(client, iss);
    else
        client->sendMessage("server 421: Unknown command from user: " + client->getNickname() + " (" + command + ")\r\n");
}

void Server::processTopic(Client *client, std::istringstream &iss)
{
    std::string target;
    iss >> target; // extract channel name
    if (target.empty())
    {
        client->sendMessage("server 461: sent not enough parameters for TOPIC\r\n");
        return ;
    }
    if (target[0] != '#' || target.length() < 2)
    {
        client->sendMessage("server 403: sent invalid characters for channel name to TOPIC\r\n");
        return ;
    }
    Channel *channel;
    if (_channels.find(target) == _channels.end())
    {
        client->sendMessage("This channel doesn't exist!\r\n");
        return ;
    }
    channel = _channels[target];
    if (!channel->isClientInChannel(client))
    {
        client->sendMessage("You don't have access to this channel!\r\n");
        return ;
    }
    std::string new_topic;
    std::getline(iss, new_topic);
    while (!new_topic.empty() && isspace(new_topic[0]))
        new_topic = new_topic.substr(1);
    if (new_topic.empty())
    {
        if (!channel->getTopic().empty())
            client->sendMessage(channel->getTopic() + "\r\n");
        else
            client->sendMessage("Topic of this channel is not set yet\r\n");
        return ;
    }
    if (!channel->isOperator(client))
    {
        client->sendMessage("You don't have operator rights to change the topic\r\n");
        return ;
    }
    channel->setTopic(new_topic);
    std::string formatted_msg = client->getNickname() + "!" + client->getUsername() + "@"
                                    + client->getHostname() + " set new topic for channel " + target + " :"
                                    + new_topic + "\r\n";
    channel->broadcast(formatted_msg, client);
}

// void Server::processInvite(Client *client, std::istringstream &iss)
// {
//     std::string target;
//     iss >> target; // extract user
//      if (target.empty())
//     {
//         client->sendMessage("server 461: sent not enough parameters for INVITE\r\n");
//         return ;
//     }
// }

void Server::processJoin(Client *client, std::istringstream &iss)
{
    bool the_first_one = false;
    std::string target;
    iss >> target; // extract channel name
    if (target.empty())
    {
        client->sendMessage("server 461: sent not enough parameters for JOIN\r\n");
        return ;
    }
    if (target[0] != '#' || target.length() < 2)
    {
        client->sendMessage("server 403: sent invalid characters for channel name to JOIN\r\n");
        return ;
    }
    Channel *channel;
    if (_channels.find(target) == _channels.end())
    {
        channel = new Channel(target);
        _channels[target] = channel;
        the_first_one = true;
        std::cout << "New channel was created: " << target << " by user " << client->getNickname() << std::endl;
    }
    else
        channel = _channels[target];
    if (channel->isClientInChannel(client))
    {
        client->sendMessage("You're already in this channel\r\n");
        return ;
    }
    if (the_first_one)
        channel->addOperator(client);
    else
        channel->addClient(client);
    client->addChannel(target);
    std::string formatted_msg = client->getNickname() + "!" + client->getUsername() + "@"
                                    + client->getHostname() + " JOIN " + target + "\r\n";
    client->sendMessage("Welcome to " + target + " channel!\r\n");
    channel->broadcast(formatted_msg, client);
    if (!channel->getTopic().empty())
        client->sendMessage(channel->getTopic() + "\r\n");
    else
        client->sendMessage("Topic of this channel is not set yet\r\n");
    // Send name list (wrote by bot and need for debug)
    std::string names = ":server 353 " + client->getNickname() + " = " + target + " :";
    const std::vector<Client*>& clients = channel->getClients();
    for (size_t i = 0; i < clients.size(); ++i)
    {
        names += clients[i]->getNickname();
        if (i < clients.size() - 1)
            names += " ";
    }
    client->sendMessage(names + "\r\n");
    client->sendMessage(":server 366 " + client->getNickname() + " " + target + " :End of NAMES list\r\n");
}

void    Server::processPrivmsg(Client *client, std::istringstream &iss)
{
    std::string target;
    iss >> target; // extract channel or nickname
    std::string message;
    std::getline(iss, message);
    if (target.empty() || message.empty())
    {
        client->sendMessage("server 461: " + client->getNickname() + " sent not enough parameters (PRIVMSG)\r\n");
        return ;
    }
    std::cout << "PRIVMSG from " << client->getNickname() << " to " << target << ": " << message << std::endl;
    if (target[0] == '#')
    {
        if (_channels.find(target) == _channels.end())
        {
            client->sendMessage("server 403: " + client->getNickname() + " sent message to " + target + " :No such channel exist\r\n");
            return ;
        }
        Channel *channel = _channels[target];
        if (!channel->isClientInChannel(client))
        {
            client->sendMessage("server 404: " + client->getNickname() + " doesn't have acces to this channel - " + target);
            return ;
        }
        std::string formatted_msg = client->getNickname() + "!" + client->getUsername() + "@"
                                    + client->getHostname() + " sent message to channel " + target + " :"
                                    + message + "\r\n";
        channel->broadcast(formatted_msg, client);
    }
    else
    {
        Client *target_user = NULL;
        for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); it++)
        {
            if (it->second->getNickname().compare(target) == 0)
            {
                target_user = it->second;
                break;
            }
        }
        if (target_user == NULL)
        {
            client->sendMessage("server 401: " + target + " doesn't exist (sent by " + client->getNickname() + ")" );
            return ;
        }
        std::string formatted_msg = client->getNickname() + "!" + client->getUsername() + "@"
                                    + client->getHostname() + " sent message to user " + target + " :"
                                    + message + "\r\n";
        target_user->sendMessage(formatted_msg);
    }
}

void Server::handleCAP(Client* client, std::istringstream& iss) {
    std::string subcommand;
    iss >> subcommand;
    if (!client->isCAPNegotiation()) {
        if (subcommand == "LS") {
            client->sendMessage("CAP * LS :\r\n");
            client->setCAPNegotiation(true);
        }
        else if (subcommand == "REQ") {
            client->sendMessage("CAP * ACK :\r\n");
            client->setCAPNegotiation(true);
        }
    }
    else if (subcommand == "END") {
        client->setCAPNegotiation(false);
    }
}

void Server::processPassword(Client* client, std::istringstream& iss) {
    std::string password;
    iss >> password;
    if (client->isAuthenticated())
        Reply::alreadyRegistered(*client);
    else if (password == _password)
        client->setAuthenticated(true);
    else
        Reply::passwordMismatch(*client);
}

void Server::processNick(Client* client, std::istringstream& iss) {
    std::string nickname;
    iss >> nickname;

    bool nickname_in_use = false;
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (it->second->getNickname() == nickname)
        {
            nickname_in_use = true;
            break;
        }
    }
    if (nickname_in_use) {
        Reply::nicknameInUse(*client, nickname);
    }
    else {
        client->setNickname(nickname);
    }
}

// <username>: The user's username/ident.
// <hostname>: Usually 0 or * (ignored by most servers).
// <servername>: Usually * (ignored by most servers).
// <realname>: The user's real name (can contain spaces, must be prefixed by :).
// USER <username> 0 * :<realname>\r\n  => e.g. USER coolguy 0 * :Cool Guy
void Server::processUser(Client* client, std::istringstream& iss) {
    std::string username, hostname, servername, realname;
    iss >> username >> hostname >> servername;

    // Parse realname (can contain spaces)
    std::getline(iss, realname);
    while (realname[0] == ' ')
        realname = realname.substr(1);
    if (!realname.empty() && realname[0] == ':')
        realname = realname.substr(1);
    else if (realname.empty()) {
        Reply::unknownCommand(*client, "USER");
        return;
    }
    else {
        Reply::needMoreParams(*client, "USER");
        return;
    }
    client->setUsername(username);
    client->setRealname(realname);
    if (!client->getNickname().empty())
        client->setRegistered(true);
}

void Server::removeClient(int clientfd)
{
    if (_clients.find(clientfd) == _clients.end())
        return;
    Client *client = _clients[clientfd];
    // Remove from channels
    for (std::vector<std::string>::iterator it = client->getChannels().begin(); it != client->getChannels().end(); ++it)
    {
        Channel *channel = _channels[*it];
        if (channel)
            channel->removeClient(client);
    }
    // Remove from pollfds
    for (std::vector<pollfd>::iterator it = _pollfds.begin(); it != _pollfds.end(); ++it)
    {
        if (it->fd == clientfd)
        {
            _pollfds.erase(it);
            break;
        }
    }
    // Close socket and delete client
    close(clientfd);
    std::cout << "Client disconnected (fd: " << clientfd << ")" << std::endl;
    delete client;
    _clients.erase(clientfd);
}

std::map<int, Client*>& Server::getClients()
{
    return _clients;
}

std::map<std::string, Channel*>& Server::getChannels()
{
    return _channels;
}

const std::string& Server::getPassword() const
{
    return _password;
}
