#include <gtest/gtest.h>
#include "../../inc/Channel.hpp"
#include "../../inc/Client.hpp"
#include "../../inc/IRCCommand.hpp"
#include "../../inc/ChannelsClientsManager.hpp"
#include "../../inc/Reply.hpp"
#include <sys/socket.h>
#include <unistd.h>

#include <fcntl.h>
#include <errno.h>

TEST(ChannelsClientsManagerTest, JoinWithoutRegistering) {
	int sv[2];
	ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0); // sv[0] and sv[1] are connected sockets
	ChannelsClientsManager manager;
	std::map<int, Client*> clients_map;
	Client* client_1 = new Client(sv[1]); // use one end for the client
	client_1->addToBuffer("PASS wrong_password\r\n"); // Make sure to use CRLF for IRC
	clients_map[1] = client_1;
	std::string password = "correct_password";
	manager.setClientsMap(&clients_map, &password, NULL);
	manager.handleClientMessage(client_1);
	char buffer[1024] = {0};
	close(sv[1]); // Close the write end after sending
	ssize_t n = read(sv[0], buffer, sizeof(buffer) - 1);
	ASSERT_GT(n, 0); // Ensure something was written
	std::string output(buffer);
	EXPECT_EQ(client_1->isAuthenticated(), false);
	EXPECT_NE(output.find("Password incorrect."), std::string::npos);
	close(sv[0]);
}

void setSocketPair(int sv[2]) {
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1) {
        perror("socketpair");
    }
	int flags = fcntl(sv[0], F_GETFL, 0);
	fcntl(sv[0], F_SETFL, flags | O_NONBLOCK);
}

ssize_t recv_nonblocking(int fd, char* buffer, size_t size) {
	ssize_t n = recv(fd, buffer, size, 0);
	if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
		// No data available
		buffer[0] = '\0';
		return 0;
	}
	else {
		buffer[n] = '\0';
	}
	return n;
}

TEST(ChannelsClientsManagerTest, JoinWithCorrectPassword) {
	int sv[2];
	setSocketPair(sv);
	// ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0); // sv[0] and sv[1] are connected sockets
	ChannelsClientsManager manager;
	std::map<int, Client*> clients_map;
	Client* client_1 = new Client(sv[1]); // use one end for the client
	client_1->addToBuffer("PASS correct_password\r\n"); // Make sure to use CRLF for IRC
	clients_map[sv[1]] = client_1;
	std::string password = "correct_password";
	manager.setClientsMap(&clients_map, &password, NULL);
	manager.handleClientMessage(client_1);
	char buffer[1024] = {0};
	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_EQ(n, 0); // Ensure something was written
	std::string output(buffer);
	EXPECT_NE(output.find(""), std::string::npos);
	EXPECT_EQ(client_1->isAuthenticated(), true);
	EXPECT_EQ(client_1->isNicknameSet(), false);
	client_1->addToBuffer("NICK testuser\r\n");
	manager.handleClientMessage(client_1);
	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_EQ(n, 0); // Ensure something was written
	std::string output2(buffer);
	EXPECT_NE(output2.find(""), std::string::npos);
	EXPECT_EQ(client_1->isNicknameSet(), true);
	EXPECT_EQ(client_1->getNickname(), "testuser");
	EXPECT_EQ(client_1->isRegistered(), false);

	// testing duplicate nick
	int sv2[2];
	setSocketPair(sv2);
	Client* client_2 = new Client(sv2[1]); // use one end for the client
	client_2->addToBuffer("PASS correct_password\r\nNICK testuser\r\n");
	clients_map[sv2[1]] = client_2;
	manager.handleClientMessage(client_2);
	n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0); // Ensure something was written
	std::string output3 = buffer;
	EXPECT_EQ(output3, ":ft_irc.42.de 433 * :testuser Nickname is already in use\r\n");
	EXPECT_EQ(client_2->isNicknameSet(), false);
	EXPECT_EQ(client_2->isAuthenticated(), true);
	EXPECT_EQ(client_2->isRegistered(), false);
	client_2->clearBuffer();
	client_2->addToBuffer("NICK testuser2\r\n");
	manager.handleClientMessage(client_2);
	n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	EXPECT_EQ(n, 0); // Ensure something was written
	std::string output4 = buffer;
	EXPECT_NE(output4.find(""), std::string::npos);
	EXPECT_EQ(client_2->isNicknameSet(), true);
	EXPECT_EQ(client_2->getNickname(), "testuser2");
	EXPECT_EQ(client_2->isRegistered(), false);


	// Now send USER command to complete registration
	client_1->addToBuffer("USER testuser 0 * :Real Name\r\n");
	manager.handleClientMessage(client_1);
	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0); // Ensure something was written
	std::string output5 = buffer;
	EXPECT_EQ(output5, ":ft_irc.42.de 001 testuser :Welcome to the ft_IRC Network, testuser\r\n");
	EXPECT_EQ(client_1->isRegistered(), true);



	close(sv[0]);
	close(sv[1]);
}


// test CAP LS / ACK negotiation
TEST(ChannelsClientsManagerTest, CapLsAckNegotiation) {
	int sv[2];
	setSocketPair(sv);
	ChannelsClientsManager manager;
	std::map<int, Client*> clients_map;
	Client* client_1 = new Client(sv[1]); // use one end for the client
	client_1->addToBuffer("PASS correct_password\r\nCAP LS\r\nNICK testuser\r\nUSER testuser 0 * :Real Name\r\n");
	clients_map[sv[1]] = client_1;
	std::string password = "correct_password";
	manager.setClientsMap(&clients_map, &password, NULL);
	manager.handleClientMessage(client_1);
	char buffer[1024] = {0};
	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0); // Ensure something was written
	std::string output(buffer);
	EXPECT_NE(output.find(""), std::string::npos);
	EXPECT_EQ(client_1->isRegistered(), false);
	client_1->clearBuffer();
	client_1->addToBuffer("CAP END\r\n");
	manager.handleClientMessage(client_1);
	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0); // Ensure something was written
	std::string output2(buffer);
	EXPECT_EQ(output2, ":ft_irc.42.de 001 testuser :Welcome to the ft_IRC Network, testuser\r\n");
	EXPECT_EQ(client_1->isRegistered(), true);
	close(sv[0]);
	close(sv[1]);
}


TEST(ChannelsClientsManagerTest, PassUserAfterRegisering) {
	int sv[2];
	setSocketPair(sv);
	ChannelsClientsManager manager;
	std::map<int, Client*> clients_map;
	Client* client_1 = new Client(sv[1]); // use one end for the client
	client_1->addToBuffer("PASS correct_password\r\nNICK testuser\r\nUSER testuser 0 * :Real Name\r\n");
	clients_map[sv[1]] = client_1;
	std::string password = "correct_password";
	manager.setClientsMap(&clients_map, &password, NULL);
	manager.handleClientMessage(client_1);
	char buffer[1024] = {0};
	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0); // Ensure something was written
	std::string output(buffer);
	EXPECT_NE(output.find(""), std::string::npos);
	EXPECT_EQ(client_1->isRegistered(), true);
	client_1->clearBuffer();
	client_1->addToBuffer("PASS correct_password\r\n");
	manager.handleClientMessage(client_1);
	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0); // No response expected
	std::string output2(buffer);
	EXPECT_EQ(output2, ":ft_irc.42.de 462 testuser :You may not reregister\r\n");
	EXPECT_EQ(client_1->isRegistered(), true); // Still registered
	// valid User second time
	client_1->clearBuffer();
	client_1->addToBuffer("USER testuser 0 * :Real Name\r\n");
	manager.handleClientMessage(client_1);
	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0); // No response expected
	std::string output3(buffer);
	EXPECT_EQ(output3, ":ft_irc.42.de 462 testuser :You may not reregister\r\n");
	EXPECT_EQ(client_1->isRegistered(), true); // Still registered

	// password second time
	client_1->clearBuffer();
	client_1->addToBuffer("PASS correct_password\r\n");
	manager.handleClientMessage(client_1);
	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0); // No response expected
	std::string output4(buffer);
	EXPECT_EQ(output4, ":ft_irc.42.de 462 testuser :You may not reregister\r\n");
	EXPECT_EQ(client_1->isRegistered(), true); // Still registered

	// setting nick second time
	client_1->clearBuffer();
	client_1->addToBuffer("NICK newnick\r\n");
	manager.handleClientMessage(client_1);
	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	close(sv[0]);
	close(sv[1]);
}

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
