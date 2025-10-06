#include "../inc/ft_irc.hpp"

Server::Server(int port, const std::string& password)
    : _port(port), _password(password)
{
    // Create socket
	// AF_INET for IPv4, SOCK_STREAM for TCP
    _socket = socket(AF_INET, SOCK_STREAM, 0);
    if (_socket < 0)
        throw std::runtime_error("Failed to create socket");
    
    // Set socket options
	// SOL_SOCKET to manipulate options at the sockets API level
	// SO_REUSEADDR to allow reuse of local addresses
	// opt = 1 to enable the option
    int opt = 1;
    if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        close(_socket);
        throw std::runtime_error("Failed to set socket options");
    }
    
    // Set non-blocking mode
    if (fcntl(_socket, F_SETFL, O_NONBLOCK) < 0)
    {
        close(_socket);
        throw std::runtime_error("Failed to set non-blocking mode");
    }

    // Initialize address structure
    memset(&_address, 0, sizeof(_address));
    _address.sin_family = AF_INET;
    _address.sin_addr.s_addr = INADDR_ANY;
    _address.sin_port = htons(_port);

    // Bind socket to address
    if (bind(_socket, (struct sockaddr *)&_address, sizeof(_address)) < 0)
    {
        close(_socket);
        throw std::runtime_error("Failed to bind socket");
    }

    // Listen for connections
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
    while (client->hasCompleteMessage())
    {
        std::string message = client->getNextMessage();
        std::cout << "Received message from client " << clientfd << ": " << message << std::endl;
        
        // Parse message
        std::istringstream iss(message);
        std::string command;
        iss >> command;
        
        // Handle command (basic example - would need to be expanded for a full IRC server)
        if (command == "PASS")
        {
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
        else if (command == "NICK")
        {
            if (!client->isAuthenticated())
            {
                client->sendMessage(":server 464 * :You must send PASS first\\r\\n");
                continue;
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
        else if (command == "USER")
        {
            if (!client->isAuthenticated())
            {
                client->sendMessage(":server 464 * :You must send PASS first\\r\\n");
                continue;
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
        else if (command == "PING")
        {
            std::string token;
            iss >> token;
            client->sendMessage("PONG " + token + "\\r\\n");
        }
        else if (command == "QUIT")
        {
            std::string reason;
            std::getline(iss, reason);
            if (!reason.empty() && reason[0] == ':')
                reason = reason.substr(1);
            
            // Broadcast quit message to channels
            std::string quit_msg = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname() 
                + " QUIT :" + (reason.empty() ? "Client Quit" : reason) + "\\r\\n";
            
            for (std::vector<std::string>::iterator it = client->getChannels().begin(); it != client->getChannels().end(); ++it)
            {
                Channel *channel = _channels[*it];
                if (channel)
                    channel->broadcast(quit_msg, client);
            }
            
            removeClient(clientfd);
            return;
        }
        // Add more command handlers here for a complete IRC server
    }
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