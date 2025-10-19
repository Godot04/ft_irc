#include "../inc/ft_irc.hpp"
#include "Reply.hpp"
#include <signal.h>
#include <csignal>

static volatile sig_atomic_t g_terminate = 0;
static void handle_sigint(int)
{
    g_terminate = 1;
}

Server::Server(int port, const std::string& password, time_t timeToLive)
    : _port(port), _password(password), _clientTimeToLive(timeToLive), _manager(_clients, _password, _pollfds)
{
    std::signal(SIGINT, handle_sigint);
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

    // _manager.setClientsMap(&_clients, &_password, &_pollfds);
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

    // Close server socket
    close(_socket);
    std::cout << "Server shut down" << std::endl;
}

void Server::start()
{
    std::cout << "Server started. Waiting for connections..." << std::endl;

    while (true)
    {
        if (g_terminate) // break quickly if signal already received
            break;
        if (poll(&_pollfds[0], _pollfds.size(), 60000) < 0) /// exit after period of time in milliseconds
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
                Client *client = _clients[_pollfds[i].fd];
                if (_pollfds[i].fd != _socket && client != NULL)
                    _manager.removeClient(*client);
            }

            Client *client = _clients[_pollfds[i].fd];
            if (client != NULL) {
                if (client->getTimePassed() >= _clientTimeToLive)
                    _manager.removeClient(*client);
                else if (client->getTimePassed() >= _clientTimeToLive / 2 && SEND_PING_AT_HALF_TIME)
                {
                    _manager.sendPingToClient(client);
                }
                continue;
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
    // std::cout << "ClientFD is: " << clientfd << std::endl;
    // std::cout << "Bytes read: " << bytes_read << std::endl;
    if (bytes_read <= 0)
    {
        if (bytes_read == 0 || errno != EAGAIN)
        {
            Client *client = _clients[clientfd];
            if (client)
                _manager.removeClient(*client);
            return;
        }
    }
    // std::cout << "I went through here\n";
    client->addToBuffer(std::string(buffer, bytes_read));
    if (!client->hasCompleteMessage())
    {
        return ; // Wait for more data
    }
    _manager.handleClientMessage(client);

    client->clearBuffer();
    if (PRINT_CLIENT_INFO && client->isRegistered())
        client->printClientInfo();
}
