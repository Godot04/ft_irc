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

// join channels

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


// Join a channel after registering
TEST(ChannelsClientsManagerTest, JoinChannelAfterRegistering) {
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
	client_1->addToBuffer("JOIN #testchannel\r\n");
	manager.handleClientMessage(client_1);
	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0); // Ensure something was written
	std::string output2(buffer);
	EXPECT_NE(output2.find("Welcome to #testchannel channel!"), std::string::npos);
	Channel* testChannel = manager.getChannel("#testchannel");
	bool clientStatus = testChannel->isClientInChannel(client_1);
	EXPECT_EQ(clientStatus, true);
	EXPECT_TRUE(testChannel->isOperator(client_1)); // First user should be operator

	close(sv[0]);
	close(sv[1]);
}

// Test JOIN with multiple channels
TEST(ChannelsClientsManagerTest, JoinMultipleChannels) {
	int sv[2];
	setSocketPair(sv);
	ChannelsClientsManager manager;
	std::map<int, Client*> clients_map;
	Client* client_1 = new Client(sv[1]);
	client_1->addToBuffer("PASS correct_password\r\nNICK testuser\r\nUSER testuser 0 * :Real Name\r\n");
	clients_map[sv[1]] = client_1;
	std::string password = "correct_password";
	manager.setClientsMap(&clients_map, &password, NULL);
	manager.handleClientMessage(client_1);
	char buffer[1024] = {0};
	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	EXPECT_EQ(client_1->isRegistered(), true);

	client_1->clearBuffer();
	client_1->addToBuffer("JOIN #channel1,#channel2,#channel3\r\n");
	manager.handleClientMessage(client_1);
	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output(buffer);
	EXPECT_NE(output.find("Welcome to #channel1"), std::string::npos);
	EXPECT_NE(output.find("Welcome to #channel2"), std::string::npos);
	EXPECT_NE(output.find("Welcome to #channel3"), std::string::npos);

	// Verify all channels were created
	Channel* ch1 = manager.getChannel("#channel1");
	Channel* ch2 = manager.getChannel("#channel2");
	Channel* ch3 = manager.getChannel("#channel3");
	EXPECT_TRUE(ch1 != nullptr);
	EXPECT_TRUE(ch2 != nullptr);
	EXPECT_TRUE(ch3 != nullptr);
	EXPECT_TRUE(ch1->isClientInChannel(client_1));
	EXPECT_TRUE(ch2->isClientInChannel(client_1));
	EXPECT_TRUE(ch3->isClientInChannel(client_1));

	close(sv[0]);
	close(sv[1]);
}

// Test JOIN with invalid channel name
TEST(ChannelsClientsManagerTest, JoinInvalidChannelName) {
	int sv[2];
	setSocketPair(sv);
	ChannelsClientsManager manager;
	std::map<int, Client*> clients_map;
	Client* client_1 = new Client(sv[1]);
	client_1->addToBuffer("PASS correct_password\r\nNICK testuser\r\nUSER testuser 0 * :Real Name\r\n");
	clients_map[sv[1]] = client_1;
	std::string password = "correct_password";
	manager.setClientsMap(&clients_map, &password, NULL);
	manager.handleClientMessage(client_1);
	char buffer[1024] = {0};
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	client_1->clearBuffer();
	client_1->addToBuffer("JOIN invalidchannel\r\n");
	manager.handleClientMessage(client_1);
	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output(buffer);
	EXPECT_NE(output.find("403"), std::string::npos); // ERR_NOSUCHCHANNEL

	close(sv[0]);
	close(sv[1]);
}

// Test joining the same channel twice
TEST(ChannelsClientsManagerTest, JoinSameChannelTwice) {
	int sv[2];
	setSocketPair(sv);
	ChannelsClientsManager manager;
	std::map<int, Client*> clients_map;
	Client* client_1 = new Client(sv[1]);
	client_1->addToBuffer("PASS correct_password\r\nNICK testuser\r\nUSER testuser 0 * :Real Name\r\n");
	clients_map[sv[1]] = client_1;
	std::string password = "correct_password";
	manager.setClientsMap(&clients_map, &password, NULL);
	manager.handleClientMessage(client_1);
	char buffer[1024] = {0};
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	client_1->clearBuffer();
	client_1->addToBuffer("JOIN #testchannel\r\n");
	manager.handleClientMessage(client_1);
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	// Try joining again
	client_1->clearBuffer();
	client_1->addToBuffer("JOIN #testchannel\r\n");
	manager.handleClientMessage(client_1);
	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output(buffer);
	EXPECT_NE(output.find("already in this channel"), std::string::npos);

	close(sv[0]);
	close(sv[1]);
}

// PRIVMSG Tests
TEST(ChannelsClientsManagerTest, PrivmsgToChannel) {
	int sv[2], sv2[2];
	setSocketPair(sv);
	setSocketPair(sv2);
	ChannelsClientsManager manager;
	std::map<int, Client*> clients_map;

	// Register first client
	Client* client_1 = new Client(sv[1]);
	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\n");
	clients_map[sv[1]] = client_1;
	std::string password = "correct_password";
	manager.setClientsMap(&clients_map, &password, NULL);
	manager.handleClientMessage(client_1);
	char buffer[1024] = {0};
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	// Register second client
	Client* client_2 = new Client(sv2[1]);
	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\n");
	clients_map[sv2[1]] = client_2;
	manager.handleClientMessage(client_2);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

	// Both join channel
	client_1->clearBuffer();
	client_1->addToBuffer("JOIN #testchannel\r\n");
	manager.handleClientMessage(client_1);
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	client_2->clearBuffer();
	client_2->addToBuffer("JOIN #testchannel\r\n");
	manager.handleClientMessage(client_2);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1); // Clear user1's buffer from user2 joining

	// Send message from user1
	client_1->clearBuffer();
	client_1->addToBuffer("PRIVMSG #testchannel :Hello everyone!\r\n");
	manager.handleClientMessage(client_1);

	// Check user2 received the message
	ssize_t n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output(buffer);
	EXPECT_NE(output.find("user1"), std::string::npos);
	EXPECT_NE(output.find("PRIVMSG #testchannel"), std::string::npos);
	EXPECT_NE(output.find("Hello everyone!"), std::string::npos);

	close(sv[0]);
	close(sv[1]);
	close(sv2[0]);
	close(sv2[1]);
}

TEST(ChannelsClientsManagerTest, PrivmsgToUser) {
	int sv[2], sv2[2];
	setSocketPair(sv);
	setSocketPair(sv2);
	ChannelsClientsManager manager;
	std::map<int, Client*> clients_map;

	// Register both clients
	Client* client_1 = new Client(sv[1]);
	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\n");
	clients_map[sv[1]] = client_1;
	std::string password = "correct_password";
	manager.setClientsMap(&clients_map, &password, NULL);
	manager.handleClientMessage(client_1);
	char buffer[1024] = {0};
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	Client* client_2 = new Client(sv2[1]);
	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\n");
	clients_map[sv2[1]] = client_2;
	manager.handleClientMessage(client_2);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

	// Send private message from user1 to user2
	client_1->clearBuffer();
	client_1->addToBuffer("PRIVMSG user2 :Hello user2!\r\n");
	manager.handleClientMessage(client_1);

	// Check user2 received the message
	ssize_t n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output(buffer);
	EXPECT_NE(output.find("user1"), std::string::npos);
	EXPECT_NE(output.find("PRIVMSG user2"), std::string::npos);
	EXPECT_NE(output.find("Hello user2!"), std::string::npos);

	close(sv[0]);
	close(sv[1]);
	close(sv2[0]);
	close(sv2[1]);
}

TEST(ChannelsClientsManagerTest, PrivmsgToNonExistentUser) {
	int sv[2];
	setSocketPair(sv);
	ChannelsClientsManager manager;
	std::map<int, Client*> clients_map;
	Client* client_1 = new Client(sv[1]);
	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\n");
	clients_map[sv[1]] = client_1;
	std::string password = "correct_password";
	manager.setClientsMap(&clients_map, &password, NULL);
	manager.handleClientMessage(client_1);
	char buffer[1024] = {0};
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	client_1->clearBuffer();
	client_1->addToBuffer("PRIVMSG nonexistent :Hello\r\n");
	manager.handleClientMessage(client_1);
	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output(buffer);
	EXPECT_NE(output.find("401"), std::string::npos); // ERR_NOSUCHNICK

	close(sv[0]);
	close(sv[1]);
}

// TOPIC Tests
TEST(ChannelsClientsManagerTest, TopicViewAndSet) {
	int sv[2], sv2[2];
	setSocketPair(sv);
	setSocketPair(sv2);
	ChannelsClientsManager manager;
	std::map<int, Client*> clients_map;
	std::string password = "correct_password";
	manager.setClientsMap(&clients_map, &password, NULL);

	// Register first client
	Client* client_1 = new Client(sv[1]);
	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\n");
	clients_map[sv[1]] = client_1;
	manager.handleClientMessage(client_1);
	char buffer[1024] = {0};
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	// Register second client
	Client* client_2 = new Client(sv2[1]);
	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\n");
	clients_map[sv2[1]] = client_2;
	manager.handleClientMessage(client_2);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

	// Both join channel
	client_1->clearBuffer();
	client_1->addToBuffer("JOIN #testchannel\r\n");
	manager.handleClientMessage(client_1);
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	client_2->clearBuffer();
	client_2->addToBuffer("JOIN #testchannel\r\n");
	manager.handleClientMessage(client_2);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1); // Clear user1's buffer from user2 joining

	// View topic (should be not set)
	client_1->clearBuffer();
	client_1->addToBuffer("TOPIC #testchannel\r\n");
	manager.handleClientMessage(client_1);
	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output(buffer);
	EXPECT_NE(output.find("not set yet"), std::string::npos);

	// Set topic (user1 is operator as first user)
	client_1->clearBuffer();
	client_1->addToBuffer("TOPIC #testchannel :New channel topic\r\n");
	manager.handleClientMessage(client_1);

	// Check that user2 received the topic change notification
	n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	output = buffer;
	EXPECT_NE(output.find("TOPIC #testchannel"), std::string::npos);
	EXPECT_NE(output.find("New channel topic"), std::string::npos);

	// View topic again from user1
	client_1->clearBuffer();
	client_1->addToBuffer("TOPIC #testchannel\r\n");
	manager.handleClientMessage(client_1);
	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	output = buffer;
	EXPECT_NE(output.find("New channel topic"), std::string::npos);

	close(sv[0]);
	close(sv[1]);
	close(sv2[0]);
	close(sv2[1]);
}

TEST(ChannelsClientsManagerTest, TopicSetWithoutOperator) {
	int sv[2], sv2[2];
	setSocketPair(sv);
	setSocketPair(sv2);
	ChannelsClientsManager manager;
	std::map<int, Client*> clients_map;
	std::string password = "correct_password";
	manager.setClientsMap(&clients_map, &password, NULL);

	// Register and join first client (will be operator)
	Client* client_1 = new Client(sv[1]);
	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\nJOIN #testchannel\r\n");
	clients_map[sv[1]] = client_1;
	manager.handleClientMessage(client_1);
	char buffer[1024] = {0};
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	// Register and join second client (not operator)
	Client* client_2 = new Client(sv2[1]);
	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\nJOIN #testchannel\r\n");
	clients_map[sv2[1]] = client_2;
	manager.handleClientMessage(client_2);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

	// Try to set topic as non-operator
	client_2->clearBuffer();
	client_2->addToBuffer("TOPIC #testchannel :Unauthorized topic\r\n");
	manager.handleClientMessage(client_2);
	ssize_t n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output(buffer);
	EXPECT_NE(output.find("operator rights"), std::string::npos);

	close(sv[0]);
	close(sv[1]);
	close(sv2[0]);
	close(sv2[1]);
}

// KICK Tests
TEST(ChannelsClientsManagerTest, KickUser) {
	int sv[2], sv2[2];
	setSocketPair(sv);
	setSocketPair(sv2);
	ChannelsClientsManager manager;
	std::map<int, Client*> clients_map;
	std::string password = "correct_password";
	manager.setClientsMap(&clients_map, &password, NULL);

	// Register both clients and join channel
	Client* client_1 = new Client(sv[1]);
	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\nJOIN #testchannel\r\n");
	clients_map[sv[1]] = client_1;
	manager.handleClientMessage(client_1);
	char buffer[1024] = {0};
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	Client* client_2 = new Client(sv2[1]);
	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\nJOIN #testchannel\r\n");
	clients_map[sv2[1]] = client_2;
	manager.handleClientMessage(client_2);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1); // Clear user1's buffer

	// Kick user2
	client_1->clearBuffer();
	client_1->addToBuffer("KICK #testchannel user2 :Bad behavior\r\n");
	manager.handleClientMessage(client_1);

	// Check user2 was kicked
	ssize_t n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output(buffer);
	EXPECT_NE(output.find("kicked"), std::string::npos);
	EXPECT_NE(output.find("Bad behavior"), std::string::npos);

	// Verify user2 is no longer in channel
	Channel* ch = manager.getChannel("#testchannel");
	EXPECT_FALSE(ch->isClientInChannel(client_2));

	close(sv[0]);
	close(sv[1]);
	close(sv2[0]);
	close(sv2[1]);
}

TEST(ChannelsClientsManagerTest, KickWithoutOperator) {
	int sv[2], sv2[2];
	setSocketPair(sv);
	setSocketPair(sv2);
	ChannelsClientsManager manager;
	std::map<int, Client*> clients_map;
	std::string password = "correct_password";
	manager.setClientsMap(&clients_map, &password, NULL);

	Client* client_1 = new Client(sv[1]);
	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\nJOIN #testchannel\r\n");
	clients_map[sv[1]] = client_1;
	manager.handleClientMessage(client_1);
	char buffer[1024] = {0};
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	Client* client_2 = new Client(sv2[1]);
	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\nJOIN #testchannel\r\n");
	clients_map[sv2[1]] = client_2;
	manager.handleClientMessage(client_2);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

	// Try to kick as non-operator
	client_2->clearBuffer();
	client_2->addToBuffer("KICK #testchannel user1 :Unauthorized kick\r\n");
	manager.handleClientMessage(client_2);
	ssize_t n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output(buffer);
	EXPECT_NE(output.find("482"), std::string::npos); // ERR_CHANOPRIVSNEEDED

	close(sv[0]);
	close(sv[1]);
	close(sv2[0]);
	close(sv2[1]);
}

// INVITE Tests
TEST(ChannelsClientsManagerTest, InviteUser) {
	int sv[2], sv2[2];
	setSocketPair(sv);
	setSocketPair(sv2);
	ChannelsClientsManager manager;
	std::map<int, Client*> clients_map;
	std::string password = "correct_password";
	manager.setClientsMap(&clients_map, &password, NULL);

	// Register both clients
	Client* client_1 = new Client(sv[1]);
	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\nJOIN #testchannel\r\n");
	clients_map[sv[1]] = client_1;
	manager.handleClientMessage(client_1);
	char buffer[1024] = {0};
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	Client* client_2 = new Client(sv2[1]);
	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\n");
	clients_map[sv2[1]] = client_2;
	manager.handleClientMessage(client_2);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

	// Invite user2 to channel
	client_1->clearBuffer();
	client_1->addToBuffer("INVITE user2 #testchannel\r\n");
	manager.handleClientMessage(client_1);

	// Check user2 received invite
	ssize_t n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output(buffer);
	EXPECT_NE(output.find("invited"), std::string::npos);
	EXPECT_NE(output.find("#testchannel"), std::string::npos);

	// Check user1 received confirmation
	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	output = buffer;
	EXPECT_NE(output.find("successfully invited"), std::string::npos);

	close(sv[0]);
	close(sv[1]);
	close(sv2[0]);
	close(sv2[1]);
}

TEST(ChannelsClientsManagerTest, InviteToNonExistentChannel) {
	int sv[2], sv2[2];
	setSocketPair(sv);
	setSocketPair(sv2);
	ChannelsClientsManager manager;
	std::map<int, Client*> clients_map;
	std::string password = "correct_password";
	manager.setClientsMap(&clients_map, &password, NULL);

	Client* client_1 = new Client(sv[1]);
	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\n");
	clients_map[sv[1]] = client_1;
	manager.handleClientMessage(client_1);
	char buffer[1024] = {0};
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	Client* client_2 = new Client(sv2[1]);
	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\n");
	clients_map[sv2[1]] = client_2;
	manager.handleClientMessage(client_2);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

	// Try to invite to non-existent channel
	client_1->clearBuffer();
	client_1->addToBuffer("INVITE user2 #nonexistent\r\n");
	manager.handleClientMessage(client_1);
	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output(buffer);
	EXPECT_NE(output.find("403"), std::string::npos); // Channel doesn't exist

	close(sv[0]);
	close(sv[1]);
	close(sv2[0]);
	close(sv2[1]);
}

// Complex/Edge Case Tests

// Test sending message to non-existent channel
TEST(ChannelsClientsManagerTest, PrivmsgToNonExistentChannel) {
	int sv[2];
	setSocketPair(sv);
	ChannelsClientsManager manager;
	std::map<int, Client*> clients_map;
	std::string password = "correct_password";
	manager.setClientsMap(&clients_map, &password, NULL);

	Client* client_1 = new Client(sv[1]);
	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\n");
	clients_map[sv[1]] = client_1;
	manager.handleClientMessage(client_1);
	char buffer[1024] = {0};
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	// Try to send message to non-existent channel
	client_1->clearBuffer();
	client_1->addToBuffer("PRIVMSG #nonexistent :Hello nobody!\r\n");
	manager.handleClientMessage(client_1);
	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output(buffer);
	EXPECT_NE(output.find("403"), std::string::npos); // Channel doesn't exist

	close(sv[0]);
	close(sv[1]);
}

// Test sending message to channel you're not a member of
TEST(ChannelsClientsManagerTest, PrivmsgToChannelNotMember) {
	int sv[2], sv2[2];
	setSocketPair(sv);
	setSocketPair(sv2);
	ChannelsClientsManager manager;
	std::map<int, Client*> clients_map;
	std::string password = "correct_password";
	manager.setClientsMap(&clients_map, &password, NULL);

	// Register and join user1
	Client* client_1 = new Client(sv[1]);
	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\nJOIN #private\r\n");
	clients_map[sv[1]] = client_1;
	manager.handleClientMessage(client_1);
	char buffer[1024] = {0};
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	// Register user2 but don't join
	Client* client_2 = new Client(sv2[1]);
	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\n");
	clients_map[sv2[1]] = client_2;
	manager.handleClientMessage(client_2);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

	// Try to send message to channel user2 is not in
	client_2->clearBuffer();
	client_2->addToBuffer("PRIVMSG #private :I shouldn't be able to send this!\r\n");
	manager.handleClientMessage(client_2);
	ssize_t n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output(buffer);
	EXPECT_NE(output.find("access"), std::string::npos);

	close(sv[0]);
	close(sv[1]);
	close(sv2[0]);
	close(sv2[1]);
}

// Test sending message after being kicked
TEST(ChannelsClientsManagerTest, PrivmsgAfterBeingKicked) {
	int sv[2], sv2[2];
	setSocketPair(sv);
	setSocketPair(sv2);
	ChannelsClientsManager manager;
	std::map<int, Client*> clients_map;
	std::string password = "correct_password";
	manager.setClientsMap(&clients_map, &password, NULL);

	// Register both clients and join channel
	Client* client_1 = new Client(sv[1]);
	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\nJOIN #testchannel\r\n");
	clients_map[sv[1]] = client_1;
	manager.handleClientMessage(client_1);
	char buffer[1024] = {0};
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	Client* client_2 = new Client(sv2[1]);
	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\nJOIN #testchannel\r\n");
	clients_map[sv2[1]] = client_2;
	manager.handleClientMessage(client_2);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	// Kick user2
	client_1->clearBuffer();
	client_1->addToBuffer("KICK #testchannel user2 :You're out!\r\n");
	manager.handleClientMessage(client_1);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	// Try to send message after being kicked
	client_2->clearBuffer();
	client_2->addToBuffer("PRIVMSG #testchannel :But I want to talk!\r\n");
	manager.handleClientMessage(client_2);
	ssize_t n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output(buffer);
	EXPECT_NE(output.find("access"), std::string::npos);

	// Verify user1 didn't receive the message
	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_EQ(n, 0); // No message should be received

	close(sv[0]);
	close(sv[1]);
	close(sv2[0]);
	close(sv2[1]);
}

// Test inviting user who is already in channel
TEST(ChannelsClientsManagerTest, InviteUserAlreadyInChannel) {
	int sv[2], sv2[2];
	setSocketPair(sv);
	setSocketPair(sv2);
	ChannelsClientsManager manager;
	std::map<int, Client*> clients_map;
	std::string password = "correct_password";
	manager.setClientsMap(&clients_map, &password, NULL);

	// Register and join both clients
	Client* client_1 = new Client(sv[1]);
	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\nJOIN #testchannel\r\n");
	clients_map[sv[1]] = client_1;
	manager.handleClientMessage(client_1);
	char buffer[1024] = {0};
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	Client* client_2 = new Client(sv2[1]);
	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\nJOIN #testchannel\r\n");
	clients_map[sv2[1]] = client_2;
	manager.handleClientMessage(client_2);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	// Try to invite user2 who is already in the channel
	client_1->clearBuffer();
	client_1->addToBuffer("INVITE user2 #testchannel\r\n");
	manager.handleClientMessage(client_1);
	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output(buffer);
	EXPECT_NE(output.find("already"), std::string::npos);

	close(sv[0]);
	close(sv[1]);
	close(sv2[0]);
	close(sv2[1]);
}

// Test kicking user who is not in channel
TEST(ChannelsClientsManagerTest, KickUserNotInChannel) {
	int sv[2], sv2[2];
	setSocketPair(sv);
	setSocketPair(sv2);
	ChannelsClientsManager manager;
	std::map<int, Client*> clients_map;
	std::string password = "correct_password";
	manager.setClientsMap(&clients_map, &password, NULL);

	// Register user1 and join channel
	Client* client_1 = new Client(sv[1]);
	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\nJOIN #testchannel\r\n");
	clients_map[sv[1]] = client_1;
	manager.handleClientMessage(client_1);
	char buffer[1024] = {0};
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	// Register user2 but don't join
	Client* client_2 = new Client(sv2[1]);
	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\n");
	clients_map[sv2[1]] = client_2;
	manager.handleClientMessage(client_2);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

	// Try to kick user2 who is not in channel
	client_1->clearBuffer();
	client_1->addToBuffer("KICK #testchannel user2 :Get out!\r\n");
	manager.handleClientMessage(client_1);
	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output(buffer);
	EXPECT_NE(output.find("441"), std::string::npos); // ERR_USERNOTINCHANNEL

	close(sv[0]);
	close(sv[1]);
	close(sv2[0]);
	close(sv2[1]);
}

// Test setting topic on non-existent channel
TEST(ChannelsClientsManagerTest, TopicOnNonExistentChannel) {
	int sv[2];
	setSocketPair(sv);
	ChannelsClientsManager manager;
	std::map<int, Client*> clients_map;
	std::string password = "correct_password";
	manager.setClientsMap(&clients_map, &password, NULL);

	Client* client_1 = new Client(sv[1]);
	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\n");
	clients_map[sv[1]] = client_1;
	manager.handleClientMessage(client_1);
	char buffer[1024] = {0};
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	// Try to set topic on non-existent channel
	client_1->clearBuffer();
	client_1->addToBuffer("TOPIC #nonexistent :New topic\r\n");
	manager.handleClientMessage(client_1);
	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output(buffer);
	EXPECT_NE(output.find("doesn't exist"), std::string::npos);

	close(sv[0]);
	close(sv[1]);
}

// Test kick, rejoin, and verify operator status is lost
TEST(ChannelsClientsManagerTest, KickRejoinLoseOperator) {
	int sv[2], sv2[2];
	setSocketPair(sv);
	setSocketPair(sv2);
	ChannelsClientsManager manager;
	std::map<int, Client*> clients_map;
	std::string password = "correct_password";
	manager.setClientsMap(&clients_map, &password, NULL);

	// Register and join both clients
	Client* client_1 = new Client(sv[1]);
	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\nJOIN #testchannel\r\n");
	clients_map[sv[1]] = client_1;
	manager.handleClientMessage(client_1);
	char buffer[1024] = {0};
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	Client* client_2 = new Client(sv2[1]);
	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\nJOIN #testchannel\r\n");
	clients_map[sv2[1]] = client_2;
	manager.handleClientMessage(client_2);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	// Verify initial operator status
	Channel* ch = manager.getChannel("#testchannel");
	EXPECT_TRUE(ch->isOperator(client_1));
	EXPECT_FALSE(ch->isOperator(client_2));

	// Kick user2
	client_1->clearBuffer();
	client_1->addToBuffer("KICK #testchannel user2 :Kicked\r\n");
	manager.handleClientMessage(client_1);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	// User2 rejoins
	client_2->clearBuffer();
	client_2->addToBuffer("JOIN #testchannel\r\n");
	manager.handleClientMessage(client_2);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	// Verify user2 is back but not operator
	EXPECT_TRUE(ch->isClientInChannel(client_2));
	EXPECT_FALSE(ch->isOperator(client_2));

	// User2 should not be able to kick user1
	client_2->clearBuffer();
	client_2->addToBuffer("KICK #testchannel user1 :Revenge!\r\n");
	manager.handleClientMessage(client_2);
	ssize_t n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output(buffer);
	EXPECT_NE(output.find("482"), std::string::npos); // ERR_CHANOPRIVSNEEDED

	close(sv[0]);
	close(sv[1]);
	close(sv2[0]);
	close(sv2[1]);
}

// Test inviting non-existent user
TEST(ChannelsClientsManagerTest, InviteNonExistentUser) {
	int sv[2];
	setSocketPair(sv);
	ChannelsClientsManager manager;
	std::map<int, Client*> clients_map;
	std::string password = "correct_password";
	manager.setClientsMap(&clients_map, &password, NULL);

	Client* client_1 = new Client(sv[1]);
	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\nJOIN #testchannel\r\n");
	clients_map[sv[1]] = client_1;
	manager.handleClientMessage(client_1);
	char buffer[1024] = {0};
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	// Try to invite non-existent user
	client_1->clearBuffer();
	client_1->addToBuffer("INVITE ghostuser #testchannel\r\n");
	manager.handleClientMessage(client_1);
	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output(buffer);
	EXPECT_NE(output.find("401"), std::string::npos); // ERR_NOSUCHNICK

	close(sv[0]);
	close(sv[1]);
}

// Test multiple users sending messages simultaneously
TEST(ChannelsClientsManagerTest, MultipleUsersMessaging) {
	int sv[2], sv2[2], sv3[2];
	setSocketPair(sv);
	setSocketPair(sv2);
	setSocketPair(sv3);
	ChannelsClientsManager manager;
	std::map<int, Client*> clients_map;
	std::string password = "correct_password";
	manager.setClientsMap(&clients_map, &password, NULL);

	// Register three clients
	Client* client_1 = new Client(sv[1]);
	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\nJOIN #chat\r\n");
	clients_map[sv[1]] = client_1;
	manager.handleClientMessage(client_1);
	char buffer[1024] = {0};
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	Client* client_2 = new Client(sv2[1]);
	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\nJOIN #chat\r\n");
	clients_map[sv2[1]] = client_2;
	manager.handleClientMessage(client_2);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	Client* client_3 = new Client(sv3[1]);
	client_3->addToBuffer("PASS correct_password\r\nNICK user3\r\nUSER user3 0 * :User Three\r\nJOIN #chat\r\n");
	clients_map[sv3[1]] = client_3;
	manager.handleClientMessage(client_3);
	recv_nonblocking(sv3[0], buffer, sizeof(buffer) - 1);
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

	// User1 sends message
	client_1->clearBuffer();
	client_1->addToBuffer("PRIVMSG #chat :Hello from user1\r\n");
	manager.handleClientMessage(client_1);

	// Verify user2 and user3 received it
	ssize_t n2 = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n2, 0);
	std::string output2(buffer);
	EXPECT_NE(output2.find("Hello from user1"), std::string::npos);

	ssize_t n3 = recv_nonblocking(sv3[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n3, 0);
	std::string output3(buffer);
	EXPECT_NE(output3.find("Hello from user1"), std::string::npos);

	// User2 replies
	client_2->clearBuffer();
	client_2->addToBuffer("PRIVMSG #chat :Hi user1!\r\n");
	manager.handleClientMessage(client_2);

	// Verify user1 and user3 received it
	ssize_t n1 = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n1, 0);
	std::string output1(buffer);
	EXPECT_NE(output1.find("Hi user1!"), std::string::npos);

	n3 = recv_nonblocking(sv3[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n3, 0);
	output3 = buffer;
	EXPECT_NE(output3.find("Hi user1!"), std::string::npos);

	close(sv[0]);
	close(sv[1]);
	close(sv2[0]);
	close(sv2[1]);
	close(sv3[0]);
	close(sv3[1]);
}

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
