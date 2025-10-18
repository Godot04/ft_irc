#include <gtest/gtest.h>
#include "../../inc/Server.hpp"
#include "../../inc/Client.hpp"
#include <thread>
#include <chrono>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

// Global for signal handler
volatile sig_atomic_t keep_running = 1;

void signal_handler(int sig) {
    (void)sig;
    keep_running = 0;
}

class ServerTest : public ::testing::Test {
protected:
    std::thread server_thread;
    Server* server;
    int test_port;
    std::string test_password;

    void SetUp() override {
        // Set up signal handler to gracefully terminate the server
        signal(SIGINT, signal_handler);

        // Reset the running flag
        keep_running = 1;

        // Use a high port unlikely to be in use
        test_port = 12345;
        test_password = "correct_pass";

        // Create the server with a longer client timeout (10 seconds instead of 2)
        server = new Server(test_port, test_password, 2);

        // Start server in background thread
        server_thread = std::thread([this]() {
            try {
                // We can't directly modify Server::start(), so we'll run it for a limited time
                // The actual test server will be running during this period
                auto start_time = std::chrono::steady_clock::now();
                auto timeout = std::chrono::seconds(10); // Run for 10 seconds max to accommodate the test

                // Launch server but in a separate process so we can stop it
                pid_t server_pid = fork();

                if (server_pid == 0) {
                    // Child process - run the server
                    server->start(); // This will run indefinitely
                    exit(0);
                } else {
                    // Parent process - wait for test to complete
                    while (keep_running &&
                           std::chrono::steady_clock::now() - start_time < timeout) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }

                    // Kill the server process when tests are done
                    kill(server_pid, SIGTERM);
                    int status;
                    waitpid(server_pid, &status, 0);
                }
            } catch (const std::exception& e) {
                std::cerr << "Server error: " << e.what() << std::endl;
            }
        });

        // Give server time to start
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    void TearDown() override {
        // Signal the server thread to stop
        keep_running = 0;

        if (server_thread.joinable())
            server_thread.join();

        delete server;
    }
};


int returnClientFd(int test_port) {
    int client_fd_1 = socket(AF_INET, SOCK_STREAM, 0);
    int flags = fcntl(client_fd_1, F_GETFL, 0);
    fcntl(client_fd_1, F_SETFL, flags | O_NONBLOCK);
    // Connect to server
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(test_port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    connect(client_fd_1, (struct sockaddr*)&addr, sizeof(addr));
    return client_fd_1;
}


// // Basic test to verify server can bind to a port
// TEST_F(ServerTest, ServerCanBind) {
//     // // We'll check if we can connect to the server, which means it successfully bound to the port
//     // int client_fd_1 = socket(AF_INET, SOCK_STREAM, 0);
//     // ASSERT_GE(client_fd_1, 0);

//     // // Set non-blocking mode for the client socket
//     // int flags = fcntl(client_fd_1, F_GETFL, 0);
//     // fcntl(client_fd_1, F_SETFL, flags | O_NONBLOCK);

//     // // Connect to server
//     // struct sockaddr_in addr;
//     // memset(&addr, 0, sizeof(addr));
//     // addr.sin_family = AF_INET;
//     // addr.sin_port = htons(test_port);
//     // inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

//     // connect(client_fd_1, (struct sockaddr*)&addr, sizeof(addr));
//     int client_fd_1 = returnClientFd(test_port);
//     // Since socket is non-blocking, we need to check for connection completion
//     fd_set write_fds;
//     FD_ZERO(&write_fds);
//     FD_SET(client_fd_1, &write_fds);

//     struct timeval timeout;
//     timeout.tv_sec = 1;
//     timeout.tv_usec = 0;

//     int select_result = select(client_fd_1 + 1, NULL, &write_fds, NULL, &timeout);

//     ASSERT_GT(select_result, 0) << "Connect timed out";

//     // Check if connection was successful
//     int error = 0;
//     socklen_t len = sizeof(error);
//     int getsockopt_result = getsockopt(client_fd_1, SOL_SOCKET, SO_ERROR, &error, &len);

//     ASSERT_EQ(getsockopt_result, 0) << "getsockopt failed with error: " << strerror(errno);
//     ASSERT_EQ(error, 0) << "Connect failed with error: " << strerror(error);

//     // Connection successful - server is running and accepting connections
//     std::cout << "Successfully connected to server on port " << test_port << std::endl;

//     // Close the connection
//     close(client_fd_1);
// }

void sendMessage(int client_fd_1, const std::string& msg) {
    send(client_fd_1, msg.c_str(), msg.length(), 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

std::string receiveMessage(int client_fd_1, int *bytes_received) {
    char buffer[1024] = {0};
    int bytes = recv(client_fd_1, buffer, sizeof(buffer)-1, 0);
    if (bytes > 0) {
        *bytes_received = bytes;
        return std::string(buffer, bytes);
    }
    *bytes_received = 0;
    return "";
}

int waitForData(int client_fd_1, int timeout_sec) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(client_fd_1, &read_fds);

    struct timeval tv;
    tv.tv_sec = timeout_sec;  // timeout for select
    tv.tv_usec = 0;

    // Wait for data to be available to read
    return select(client_fd_1 + 1, &read_fds, NULL, NULL, &tv);
}

// Test simple communication with the server
TEST_F(ServerTest, BasicCommunication) {
    int bytes_received;
    int client_fd_1 = returnClientFd(test_port);
    sendMessage(client_fd_1, "PASS correct_pass\r\n");

    std::string response = receiveMessage(client_fd_1, &bytes_received);
    EXPECT_EQ(response, ":ft_irc.42.de 001  :Welcome to the ft_IRC Network\r\n");
    sendMessage(client_fd_1, "NICK testuser1\r\nUSER testuser1 0 * :Test1 User1\r\n");

    response = receiveMessage(client_fd_1, &bytes_received);
    EXPECT_EQ(response, ":ft_irc.42.de 001 testuser1 :Welcome to the ft_IRC Network\r\n");

    int select_result = waitForData(client_fd_1, 3);
    ASSERT_GT(select_result, 0) << "Timeout waiting for server response";

    std::string response2 = receiveMessage(client_fd_1, &bytes_received);
    EXPECT_EQ(response2, "Connection closed\r\n");
    close(client_fd_1);
}

TEST_F(ServerTest, ClientTimeout) {
    int bytes_received;
    int client_fd_1 = returnClientFd(test_port);
    sendMessage(client_fd_1, "PASS correct_pass\r\nNICK testuser1\r\nUSER testuser1 0 * :Test1 User1\r\n");

    std::string response = receiveMessage(client_fd_1, &bytes_received); // just to clear buffer

    // Wait for data to be available to read
    int select_result = waitForData(client_fd_1, 3);
    ASSERT_GT(select_result, 0) << "Timeout waiting for server response";

    std::string response3 = receiveMessage(client_fd_1, &bytes_received);
    EXPECT_EQ(response3, "Connection closed\r\n");

    close(client_fd_1);
}
