#include "../inc/ft_irc.hpp"

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
        return 1;
    }

    int port = std::atoi(argv[1]);
    // check if port is between 1024 and 65535??
    if (port <= 0 || port > 65535)
    {
        std::cerr << "Error: Invalid port number" << std::endl;
        return 1;
    }
    std::string password = argv[2];
    if (password.empty())
    {
        std::cerr << "Error: Password cannot be empty" << std::endl;
        return 1;
    }

    try
    {
        Server server(port, password, 40);
        server.start();
    }
    catch (std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
