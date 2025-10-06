#include "../inc/ft_irc.hpp"

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

    // Set hostname
    // inet_ntoa(client_addr.sin_addr) converts the IPv4 address in
    // client_addr.sin_addr into a human‑readable dotted string like "192.168.0.5".
    client->setHostname(inet_ntoa(client_addr.sin_addr));

    std::cout << "New connection from " << client->getHostname() << " (fd: " << client_fd << ")" << std::endl;

    // Send welcome message
    client->sendMessage(":" + std::string("ft_irc") + " NOTICE * :*** Please enter password with /PASS <password>\\r\\n");
}

void Server::handleClientMessage(int clientfd)
{
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
    
    // Add to client's buffer
    client->addToBuffer(std::string(buffer, bytes_read));
    // Process complete messages
    while (client->hasCompleteMessage()) // it is true if message contains end of line \r\n
    {
        std::string message = client->getNextMessage(); // extracts message until \r\n from buffer
        std::cout << "Received message from client " << clientfd << ": " << message << std::endl;

        // Parse message
        std::istringstream iss(message);
        std::string command;
        iss >> command;
        if (command == "PASS")
        {
            processPassword(client, iss);
        }
        else if (command == "NICK")
        {
            processNick(client, iss);
        }
        else if (command == "USER")
        {
            processUser(client, iss);
        }
        else {
            client->sendMessage(":server 421 * " + command + " :Unknown command\\r\\n");
        }
        client->clearBuffer();
        if (client->isAuthenticated() && client->isNicknameSet() && client->isUsernameSet())
        {
            std::cout << "inside channel join" << std::endl;
            if (client->getChannels().empty()) {
                std::string defaultChannel = "#general";
                Channel* channel;
                if (_channels.find(defaultChannel) == _channels.end()) {
                    channel = new Channel(defaultChannel);
                    _channels[defaultChannel] = channel;
                } else {
                    channel = _channels[defaultChannel];
                }
                channel->addClient(client);
                client->addChannel(defaultChannel);
                channel->broadcast(":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() + " JOIN :" + defaultChannel + "\r\n", client);
                client->sendMessage(":server 332 " + client->getNickname() + " " + defaultChannel + " :Welcome to the general channel\r\n");
            }
        }
        // else if (command == "PING")
        // {
        //     std::string token;
        //     iss >> token;
        //     client->sendMessage("PONG " + token + "\\r\\n");
        // }
        // else if (command == "QUIT")
        // {
        //     std::string reason;
        //     std::getline(iss, reason);
        //     if (!reason.empty() && reason[0] == ':')
        //         reason = reason.substr(1);
            
        //     // Broadcast quit message to channels
        //     std::string quit_msg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() 
        //         + " QUIT :" + (reason.empty() ? "Client Quit" : reason) + "\\r\\n";
            
        //     for (std::vector<std::string>::iterator it = client->getChannels().begin(); it != client->getChannels().end(); ++it)
        //     {
        //         Channel *channel = _channels[*it];
        //         if (channel)
        //             channel->broadcast(quit_msg, client);
        //     }
            
        //     removeClient(clientfd);
        //     return;
        // }
        // Add more command handlers here for a complete IRC server

    }
}

void Server::processPassword(Client* client, std::istringstream& iss) {
    std::string password;
    iss >> password;
    
    if (password == _password)
    {
        client->setAuthenticated(true);
        client->sendMessage(":server 001 * :Password accepted\\r\\n");
    }
    else
    {
        client->sendMessage(":server 464 * :Password incorrect\\r\\n");
    }
}

void Server::processNick(Client* client, std::istringstream& iss) {
    if (!client->isAuthenticated())
    {
        client->sendMessage(":server 464 * :You must send PASS first\\r\\n");
        return ;
    }
    
    std::string nickname;
    iss >> nickname;
    
    // Check if nickname is already in use
    bool nickname_in_use = false;
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (it->second->getNickname() == nickname)
        {
            nickname_in_use = true;
            break;
        }
    }
    
    if (nickname_in_use)
    {
        client->sendMessage(":server 433 * " + nickname + " :Nickname is already in use\\r\\n");
    }
    else
    {
        client->setNickname(nickname);
        client->sendMessage(":server 001 " + nickname + " :Welcome to the ft_irc server\\r\\n");

        if (client->getUsername().empty() == false)
            client->setRegistered(true);
    }
}

void Server::processUser(Client* client, std::istringstream& iss) {
    if (!client->isAuthenticated())
    {
        client->sendMessage(":server 464 * :You must send PASS first\\r\\n");
        return ;
    }
    
    std::string username, hostname, servername, realname;
    iss >> username >> hostname >> servername;
    
    // Parse realname (can contain spaces)
    std::getline(iss, realname);
    if (!realname.empty() && realname[0] == ':')
        realname = realname.substr(1);
    
    client->setUsername(username);
    client->setRealname(realname);
    
    if (client->getNickname().empty() == false)
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