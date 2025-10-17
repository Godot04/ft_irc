#include <gtest/gtest.h>
#include "../../inc/Channel.hpp"
#include "../../inc/Client.hpp"
#include "../../inc/IRCCommand.hpp"
#include "../../inc/ChannelsClientsManager.hpp"
#include "../../inc/Reply.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <array>
#include <fcntl.h>
#include <errno.h>

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


TEST(ChannelsClientsManagerTest, JoinWithWrongPassword) {
	int sv[2];
	setSocketPair(sv);
	std::vector<pollfd> pollfds;
	std::map<int, Client*> clients_map;
	pollfd server_pfd;
	pollfds.push_back(server_pfd);

	Client* client_1 = new Client(sv[1]);
	client_1->addToBuffer("PASS wrong_password\r\n");
	clients_map[sv[1]] = client_1;
	pollfd pfd;
	pfd.fd = sv[1];
	pfd.events = POLLIN;
	pollfds.push_back(pfd);

	std::string password = "correct_password";
	ChannelsClientsManager manager(clients_map, password, pollfds);
	manager.handleClientMessage(client_1);

	char buffer[1024] = {0};
	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	ASSERT_GT(n, 0);
	std::string output(buffer);
	EXPECT_EQ(client_1->isAuthenticated(), false);
	EXPECT_NE(output.find("Password incorrect"), std::string::npos);

	close(sv[0]);
	close(sv[1]);
}

TEST(ChannelsClientsManagerTest, RegistrationFlow) {
	int sv[2];
	setSocketPair(sv);
	std::vector<pollfd> pollfds;
	std::map<int, Client*> clients_map;
	pollfd server_pfd;
	pollfds.push_back(server_pfd);

	Client* client_1 = new Client(sv[1]);
	client_1->addToBuffer("PASS correct_password\r\n");
	clients_map[sv[1]] = client_1;
	pollfd pfd;
	pfd.fd = sv[1];
	pfd.events = POLLIN;
	pollfds.push_back(pfd);

	std::string password = "correct_password";
	ChannelsClientsManager manager(clients_map, password, pollfds);
	manager.handleClientMessage(client_1);

	char buffer[1024] = {0};
	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_EQ(n, 0);
	EXPECT_EQ(client_1->isAuthenticated(), true);
	EXPECT_EQ(client_1->isNicknameSet(), false);

	client_1->clearBuffer();
	client_1->addToBuffer("NICK testuser\r\n");
	manager.handleClientMessage(client_1);
	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_EQ(n, 0);
	EXPECT_EQ(client_1->isNicknameSet(), true);
	EXPECT_EQ(client_1->getNickname(), "testuser");
	EXPECT_EQ(client_1->isRegistered(), false);

	// testing duplicate nick
	int sv2[2];
	setSocketPair(sv2);
	Client* client_2 = new Client(sv2[1]);
	client_2->addToBuffer("PASS correct_password\r\nNICK testuser\r\n");
	clients_map[sv2[1]] = client_2;
	pollfd pfd2;
	pfd2.fd = sv2[1];
	pfd2.events = POLLIN;
	pollfds.push_back(pfd2);

	manager.handleClientMessage(client_2);
	n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output3 = buffer;
	EXPECT_NE(output3.find("433"), std::string::npos);
	EXPECT_NE(output3.find("Nickname is already in use"), std::string::npos);
	EXPECT_EQ(client_2->isNicknameSet(), false);
	EXPECT_EQ(client_2->isAuthenticated(), true);
	EXPECT_EQ(client_2->isRegistered(), false);

	client_2->clearBuffer();
	client_2->addToBuffer("NICK testuser2\r\n");
	manager.handleClientMessage(client_2);
	n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	EXPECT_EQ(n, 0);
	EXPECT_EQ(client_2->isNicknameSet(), true);
	EXPECT_EQ(client_2->getNickname(), "testuser2");
	EXPECT_EQ(client_2->isRegistered(), false);

	// Now send USER command to complete registration
	client_1->clearBuffer();
	client_1->addToBuffer("USER testuser 0 * :Real Name\r\n");
	manager.handleClientMessage(client_1);
	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output5 = buffer;
	EXPECT_NE(output5.find("Welcome"), std::string::npos);
	EXPECT_EQ(client_1->isRegistered(), true);

	close(sv[0]);
	close(sv[1]);
	close(sv2[0]);
	close(sv2[1]);
}

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Helper Function <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

Client *returnReadyToConnectClient(std::vector<pollfd>& pollfds, std::map<int, Client*>& clients_map, int sv[2], const std::string& password, const std::string& nick, const std::string& user) {
	setSocketPair(sv);
	Client* client = new Client(sv[1]); // use one end for the client
	std::string reg_msg = "PASS " + password + "\r\nNICK " + nick + "\r\nUSER " + user + " 0 * :Real Name\r\n";
	client->addToBuffer(reg_msg);
	pollfd server_pfd;
	pollfd pfd;
	pfd.fd = sv[1]; // Client socket
	pfd.events = POLLIN;
	pollfds.push_back(pfd);
	clients_map[sv[1]] = client;
	return client;
}


TEST(ChannelsClientsManagerTest, CheckManagerStructure) {
	int sv[2];
	std::string correctPass = "correct_password";
	std::vector<pollfd>             pollfds;
	std::map<int, Client*>			clients_map;
	pollfd server_pfd;
	pollfds.push_back(server_pfd);


	Client* client_1 = returnReadyToConnectClient(pollfds, clients_map, sv, correctPass, "testuser", "testuser");
	ChannelsClientsManager manager(clients_map, correctPass, pollfds);
	manager.handleClientMessage(client_1);
	ASSERT_EQ(client_1->isRegistered(), true);
	client_1->clearBuffer();
	ASSERT_EQ(manager.getPollSize(), 2);
	ASSERT_EQ(manager.getClientsSize(), 1);
	ASSERT_EQ(manager.getChannelsSize(), 0);
	int sv2[2];
	Client *client_2 = returnReadyToConnectClient(pollfds, clients_map, sv2, correctPass, "testuser2", "testuser2");
	manager.handleClientMessage(client_2);
	ASSERT_EQ(client_2->isRegistered(), true);
	ASSERT_EQ(manager.getPollSize(), 3);
	ASSERT_EQ(manager.getClientsSize(), 2);
	ASSERT_EQ(manager.getChannelsSize(), 0);
	int sv3[2];
	Client *client_3 = returnReadyToConnectClient(pollfds, clients_map, sv3, correctPass, "testuser3", "testuser3");
	manager.handleClientMessage(client_3);
	ASSERT_EQ(client_3->isRegistered(), true);
	ASSERT_EQ(manager.getPollSize(), 4);
	ASSERT_EQ(manager.getClientsSize(), 3);
	ASSERT_EQ(manager.getChannelsSize(), 0);
	manager.removeClient(*client_2);
	ASSERT_EQ(manager.getPollSize(), 3);
	ASSERT_EQ(manager.getClientsSize(), 2);
	ASSERT_EQ(manager.getChannelsSize(), 0);
	manager.removeClient(*client_1);
	manager.removeClient(*client_3);
	ASSERT_EQ(manager.getPollSize(), 1);
	ASSERT_EQ(manager.getClientsSize(), 0);
	ASSERT_EQ(manager.getChannelsSize(), 0);

	close(sv[0]);
	close(sv[1]);
}



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> PING PONG <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


TEST(ChannelsClientsManagerTest, PingPongFromClient) {
	int sv[2];
	std::string correctPass = "correct_password";
	std::vector<pollfd> pollfds;
	std::map<int, Client*> clients_map;
	pollfd server_pfd;
	pollfds.push_back(server_pfd);

	Client* client = returnReadyToConnectClient(pollfds, clients_map, sv, correctPass, "testuser", "testuser");
	ChannelsClientsManager manager(clients_map, correctPass, pollfds);

	// Complete registration
	manager.handleClientMessage(client);
	char buffer[1024] = {0};
	// Drain any registration/welcome messages
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	ASSERT_EQ(client->isRegistered(), true);

	// Send PING and expect a PONG containing the token
	client->clearBuffer();
	client->addToBuffer("PING ft_irc.42.de\r\n");
	manager.handleClientMessage(client);
	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output = buffer;
	EXPECT_EQ(output, "PONG ft_irc.42.de\r\n");

	close(sv[0]);
	close(sv[1]);
}

TEST(ChannelsClientsManagerTest, PingPongFromServer) {
	int sv[2];
	std::string correctPass = "correct_password";
	std::vector<pollfd> pollfds;
	std::map<int, Client*> clients_map;
	pollfd server_pfd;
	pollfds.push_back(server_pfd);

	Client* client = returnReadyToConnectClient(pollfds, clients_map, sv, correctPass, "testuser", "testuser");
	ChannelsClientsManager manager(clients_map, correctPass, pollfds);

	// Complete registration
	manager.handleClientMessage(client);
	char buffer[1024] = {0};
	// Drain any registration/welcome messages
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	ASSERT_EQ(client->isRegistered(), true);

	// Simulate server sending PING
	manager.sendPingToClient(client);
	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	std::string output = buffer;
	EXPECT_EQ(output, "PING ft_irc.42.de\r\n");


	close(sv[0]);
	close(sv[1]);
}


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> MODE <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
TEST(ChannelsClientsManagerTest, ModeSetInviteOnlyChannel) {
	int sv[2];
	std::string correctPass = "correct_password";
	std::vector<pollfd> pollfds;
	std::map<int, Client*> clients_map;
	pollfd server_pfd;
	pollfds.push_back(server_pfd);

	Client* client = returnReadyToConnectClient(pollfds, clients_map, sv, correctPass, "testuser", "testuser");
	ChannelsClientsManager manager(clients_map, correctPass, pollfds);

	// Complete registration
	manager.handleClientMessage(client);
	char buffer[1024] = {0};
	// Drain any registration/welcome messages
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	ASSERT_EQ(client->isRegistered(), true);

	client->clearBuffer();
	client->addToBuffer("MODE #nonexistent +i\r\n");
	manager.handleClientMessage(client);
	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output = buffer;
	EXPECT_EQ(output, ":ft_irc.42.de 403 #nonexistent : No such channel\r\n");

	// Join a channel
	client->clearBuffer();
	client->addToBuffer("JOIN #testchannel\r\n");
	manager.handleClientMessage(client);
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	ASSERT_NE(std::string(buffer).find("Welcome to #testchannel channel!"), std::string::npos);

	// Set channel to invite-only mode
	client->clearBuffer();
	client->addToBuffer("MODE #testchannel +i\r\n");
	manager.handleClientMessage(client);
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1); // Drain MODE response

	// Verify mode was set
	EXPECT_TRUE(manager.getChannel("#testchannel")->isInviteOnly());

	close(sv[0]);
	close(sv[1]);
}

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> MODE FLAG TESTS +i (invite-only) <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

TEST(ChannelsClientsManagerTest, ModeInviteOnlyPreventJoin) {
	std::string correctPass = "correct_password";
	std::vector<pollfd> pollfds;
	std::map<int, Client*> clients_map;
	pollfd server_pfd;
	pollfds.push_back(server_pfd);

	ChannelsClientsManager manager(clients_map, correctPass, pollfds);

	// Setup operator client
	int sv[2];
	Client* operator_client = returnReadyToConnectClient(pollfds, clients_map, sv, correctPass, "operator", "operator");
	manager.handleClientMessage(operator_client);
	char buffer[2048] = {0};
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	// Operator joins and creates channel
	operator_client->clearBuffer();
	operator_client->addToBuffer("JOIN #private\r\n");
	manager.handleClientMessage(operator_client);
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	// Set invite-only mode
	operator_client->clearBuffer();
	operator_client->addToBuffer("MODE #private +i\r\n");
	manager.handleClientMessage(operator_client);
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	// Verify mode is set
	EXPECT_EQ(manager.getChannel("#private")->isInviteOnly(), true);

	// Setup regular client
	int sv2[2];
	Client* regular_client = returnReadyToConnectClient(pollfds, clients_map, sv2, correctPass, "regular", "regular");
	manager.handleClientMessage(regular_client);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

	// Regular client tries to join invite-only channel - should fail with 473
	regular_client->clearBuffer();
	regular_client->addToBuffer("JOIN #private\r\n");
	manager.handleClientMessage(regular_client);
	ssize_t n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output = buffer;
	EXPECT_NE(output.find("473"), std::string::npos); // ERR_INVITEONLYCHAN

	// Verify regular client is NOT in channel
	EXPECT_FALSE(manager.getChannel("#private")->isClientInChannel(regular_client));

	close(sv[0]);
	close(sv[1]);
	close(sv2[0]);
	close(sv2[1]);
}

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> MODE FLAG TESTS +k (channel key/password) <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

TEST(ChannelsClientsManagerTest, ModeChannelKeyRequired) {
	std::string correctPass = "correct_password";
	std::vector<pollfd> pollfds;
	std::map<int, Client*> clients_map;
	pollfd server_pfd;
	pollfds.push_back(server_pfd);

	ChannelsClientsManager manager(clients_map, correctPass, pollfds);

	// Setup operator
	int sv[2];
	Client* operator_client = returnReadyToConnectClient(pollfds, clients_map, sv, correctPass, "operator", "operator");
	manager.handleClientMessage(operator_client);
	char buffer[2048] = {0};
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	// Join and set channel key
	operator_client->clearBuffer();
	operator_client->addToBuffer("JOIN #secured\r\nMODE #secured +k secret123\r\n");
	manager.handleClientMessage(operator_client);
	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

	EXPECT_EQ(manager.getChannel("#secured")->getKey(), "secret123");

	// Setup regular client
	int sv2[2];
	Client* regular_client = returnReadyToConnectClient(pollfds, clients_map, sv2, correctPass, "regular", "regular");
	manager.handleClientMessage(regular_client);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

	// Try to join WITHOUT key - should fail with 475
	regular_client->clearBuffer();
	regular_client->addToBuffer("JOIN #secured\r\n");
	manager.handleClientMessage(regular_client);
	ssize_t n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output = buffer;
	EXPECT_NE(output.find("475"), std::string::npos); // ERR_BADCHANNELKEY
	EXPECT_FALSE(manager.getChannel("#secured")->isClientInChannel(regular_client));

	// Try to join with WRONG key - should fail
	regular_client->clearBuffer();
	regular_client->addToBuffer("JOIN #secured wrongkey\r\n");
	manager.handleClientMessage(regular_client);
	n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	output = buffer;
	EXPECT_NE(output.find("475"), std::string::npos);
	EXPECT_FALSE(manager.getChannel("#secured")->isClientInChannel(regular_client));

	// Join with CORRECT key - should succeed
	regular_client->clearBuffer();
	regular_client->addToBuffer("JOIN #secured secret123\r\n");
	manager.handleClientMessage(regular_client);
	n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	output = buffer;
	EXPECT_NE(output.find("Welcome to #secured channel!"), std::string::npos);
	EXPECT_TRUE(manager.getChannel("#secured")->isClientInChannel(regular_client));

	close(sv[0]);
	close(sv[1]);
	close(sv2[0]);
	close(sv2[1]);
}

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> MODE FLAG TESTS +l (user limit) <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

TEST(ChannelsClientsManagerTest, ModeUserLimitEnforced) {
	std::string correctPass = "correct_password";
	std::vector<pollfd> pollfds;
	std::map<int, Client*> clients_map;
	pollfd server_pfd;
	pollfds.push_back(server_pfd);

	ChannelsClientsManager manager(clients_map, correctPass, pollfds);

	// Setup operator
	int sv1[2];
	Client* operator_client = returnReadyToConnectClient(pollfds, clients_map, sv1, correctPass, "operator", "operator");
	manager.handleClientMessage(operator_client);
	char buffer[2048] = {0};
	recv_nonblocking(sv1[0], buffer, sizeof(buffer) - 1);

	// Create channel and set user limit to 2
	operator_client->clearBuffer();
	operator_client->addToBuffer("JOIN #limited\r\nMODE #limited +l 2\r\n");
	manager.handleClientMessage(operator_client);
	recv_nonblocking(sv1[0], buffer, sizeof(buffer) - 1);

	EXPECT_EQ(manager.getChannel("#limited")->getUserLimit(), 2);

	// Second client joins - should succeed (2/2)
	int sv2[2];
	Client* client2 = returnReadyToConnectClient(pollfds, clients_map, sv2, correctPass, "user2", "user2");
	manager.handleClientMessage(client2);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

	client2->clearBuffer();
	client2->addToBuffer("JOIN #limited\r\n");
	manager.handleClientMessage(client2);
	ssize_t n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output = buffer;
	EXPECT_NE(output.find("Welcome to #limited channel!"), std::string::npos);
	EXPECT_TRUE(manager.getChannel("#limited")->isClientInChannel(client2));

	// Third client tries to join - should fail with 471 (channel full)
	int sv3[2];
	Client* client3 = returnReadyToConnectClient(pollfds, clients_map, sv3, correctPass, "user3", "user3");
	manager.handleClientMessage(client3);
	recv_nonblocking(sv3[0], buffer, sizeof(buffer) - 1);

	client3->clearBuffer();
	client3->addToBuffer("JOIN #limited\r\n");
	manager.handleClientMessage(client3);
	n = recv_nonblocking(sv3[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	output = buffer;
	EXPECT_NE(output.find("471"), std::string::npos); // ERR_CHANNELISFULL
	EXPECT_FALSE(manager.getChannel("#limited")->isClientInChannel(client3));

	close(sv1[0]);
	close(sv1[1]);
	close(sv2[0]);
	close(sv2[1]);
	close(sv3[0]);
	close(sv3[1]);
}

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> MODE FLAG TESTS +t (topic protected) <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

TEST(ChannelsClientsManagerTest, ModeTopicProtectionEnforced) {
	std::string correctPass = "correct_password";
	std::vector<pollfd> pollfds;
	std::map<int, Client*> clients_map;
	pollfd server_pfd;
	pollfds.push_back(server_pfd);

	ChannelsClientsManager manager(clients_map, correctPass, pollfds);

	// Setup operator
	int sv1[2];
	Client* operator_client = returnReadyToConnectClient(pollfds, clients_map, sv1, correctPass, "operator", "operator");
	manager.handleClientMessage(operator_client);
	char buffer[2048] = {0};
	recv_nonblocking(sv1[0], buffer, sizeof(buffer) - 1);

	// Create channel
	operator_client->clearBuffer();
	operator_client->addToBuffer("JOIN #tprotected\r\n");
	manager.handleClientMessage(operator_client);
	recv_nonblocking(sv1[0], buffer, sizeof(buffer) - 1);

	// Setup regular client
	int sv2[2];
	Client* regular_client = returnReadyToConnectClient(pollfds, clients_map, sv2, correctPass, "regular", "regular");
	manager.handleClientMessage(regular_client);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

	regular_client->clearBuffer();
	regular_client->addToBuffer("JOIN #tprotected\r\n");
	manager.handleClientMessage(regular_client);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	recv_nonblocking(sv1[0], buffer, sizeof(buffer) - 1); // Clear operator buffer

	// Without +t, regular user CAN change topic
	regular_client->clearBuffer();
	regular_client->addToBuffer("TOPIC #tprotected :Anyone can set this\r\n");
	manager.handleClientMessage(regular_client);
	ssize_t n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	// Should succeed (no error)

	// Now operator sets +t mode
	operator_client->clearBuffer();
	operator_client->addToBuffer("MODE #tprotected +t\r\n");
	manager.handleClientMessage(operator_client);
	recv_nonblocking(sv1[0], buffer, sizeof(buffer) - 1);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

	EXPECT_EQ(manager.getChannel("#tprotected")->isTopicProtected(), true);

	// Now regular user CANNOT change topic
	regular_client->clearBuffer();
	regular_client->addToBuffer("TOPIC #tprotected :Regular user tries to change\r\n");
	manager.handleClientMessage(regular_client);
	n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output = buffer;
	EXPECT_NE(output.find("482"), std::string::npos); // ERR_CHANOPRIVSNEEDED

	// But operator CAN still change topic
	operator_client->clearBuffer();
	operator_client->addToBuffer("TOPIC #tprotected :Operator sets new topic\r\n");
	manager.handleClientMessage(operator_client);
	// Topic change broadcasts to all channel members including operator
	n = recv_nonblocking(sv1[0], buffer, sizeof(buffer) - 1);
	// Operator should receive TOPIC broadcast
	n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	// Regular client should also receive it
	// Should succeed without error

	close(sv1[0]);
	close(sv1[1]);
	close(sv2[0]);
	close(sv2[1]);
}

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> MODE FLAG TESTS - Combined Flags <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

TEST(ChannelsClientsManagerTest, ModeCombinedFlags) {
	std::string correctPass = "correct_password";
	std::vector<pollfd> pollfds;
	std::map<int, Client*> clients_map;
	pollfd server_pfd;
	pollfds.push_back(server_pfd);

	ChannelsClientsManager manager(clients_map, correctPass, pollfds);

	// Setup operator
	int sv1[2];
	Client* operator_client = returnReadyToConnectClient(pollfds, clients_map, sv1, correctPass, "operator", "operator");
	manager.handleClientMessage(operator_client);
	char buffer[2048] = {0};
	recv_nonblocking(sv1[0], buffer, sizeof(buffer) - 1);

	// Create channel with multiple modes: +i +t +k +l
	operator_client->clearBuffer();
	operator_client->addToBuffer("JOIN #fortress\r\n");
	manager.handleClientMessage(operator_client);
	recv_nonblocking(sv1[0], buffer, sizeof(buffer) - 1);

	operator_client->clearBuffer();
	operator_client->addToBuffer("MODE #fortress +i\r\nMODE #fortress +t\r\nMODE #fortress +k pass123\r\nMODE #fortress +l 5\r\n");
	manager.handleClientMessage(operator_client);
	recv_nonblocking(sv1[0], buffer, sizeof(buffer) - 1);

	Channel* ch = manager.getChannel("#fortress");
	EXPECT_TRUE(ch->isInviteOnly());
	EXPECT_TRUE(ch->isTopicProtected());
	EXPECT_EQ(ch->getKey(), "pass123");
	EXPECT_EQ(ch->getUserLimit(), 5);

	// Setup regular client
	int sv2[2];
	Client* regular_client = returnReadyToConnectClient(pollfds, clients_map, sv2, correctPass, "regular", "regular");
	manager.handleClientMessage(regular_client);
	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

	// Try to join - should fail (invite-only)
	regular_client->clearBuffer();
	regular_client->addToBuffer("JOIN #fortress\r\n");
	manager.handleClientMessage(regular_client);
	ssize_t n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	std::string output = buffer;
	EXPECT_NE(output.find("473"), std::string::npos);

	// Try with key but still invite-only - should still fail
	regular_client->clearBuffer();
	regular_client->addToBuffer("JOIN #fortress pass123\r\n");
	manager.handleClientMessage(regular_client);
	n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	output = buffer;
	EXPECT_NE(output.find("473"), std::string::npos); // Still fails on invite-only

	// Remove invite-only but keep key
	operator_client->clearBuffer();
	operator_client->addToBuffer("MODE #fortress -i\r\n");
	manager.handleClientMessage(operator_client);
	recv_nonblocking(sv1[0], buffer, sizeof(buffer) - 1);

	// Now try with correct key - should succeed
	regular_client->clearBuffer();
	regular_client->addToBuffer("JOIN #fortress pass123\r\n");
	manager.handleClientMessage(regular_client);
	n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
	EXPECT_GT(n, 0);
	output = buffer;
	EXPECT_NE(output.find("Welcome to #fortress channel!"), std::string::npos);
	EXPECT_TRUE(ch->isClientInChannel(regular_client));

	close(sv1[0]);
	close(sv1[1]);
	close(sv2[0]);
	close(sv2[1]);
}

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> MODE FLAG TESTS - Remove Flags <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

TEST(ChannelsClientsManagerTest, ModeRemoveFlags) {
	std::string correctPass = "correct_password";
	std::vector<pollfd> pollfds;
	std::map<int, Client*> clients_map;
	pollfd server_pfd;
	pollfds.push_back(server_pfd);

	ChannelsClientsManager manager(clients_map, correctPass, pollfds);

	// Setup operator
	int sv1[2];
	Client* operator_client = returnReadyToConnectClient(pollfds, clients_map, sv1, correctPass, "operator", "operator");
	manager.handleClientMessage(operator_client);
	char buffer[2048] = {0};
	recv_nonblocking(sv1[0], buffer, sizeof(buffer) - 1);

	operator_client->clearBuffer();
	operator_client->addToBuffer("JOIN #removable\r\n");
	manager.handleClientMessage(operator_client);
	recv_nonblocking(sv1[0], buffer, sizeof(buffer) - 1);

	// Set all flags
	operator_client->clearBuffer();
	operator_client->addToBuffer("MODE #removable +i\r\nMODE #removable +t\r\nMODE #removable +k secret\r\nMODE #removable +l 10\r\n");
	manager.handleClientMessage(operator_client);
	recv_nonblocking(sv1[0], buffer, sizeof(buffer) - 1);

	Channel* ch = manager.getChannel("#removable");
	EXPECT_TRUE(ch->isInviteOnly());
	EXPECT_TRUE(ch->isTopicProtected());
	EXPECT_EQ(ch->getKey(), "secret");
	EXPECT_EQ(ch->getUserLimit(), 10);

	// Remove invite-only
	operator_client->clearBuffer();
	operator_client->addToBuffer("MODE #removable -i\r\n");
	manager.handleClientMessage(operator_client);
	recv_nonblocking(sv1[0], buffer, sizeof(buffer) - 1);
	EXPECT_FALSE(ch->isInviteOnly());

	// Remove topic protection
	operator_client->clearBuffer();
	operator_client->addToBuffer("MODE #removable -t\r\n");
	manager.handleClientMessage(operator_client);
	recv_nonblocking(sv1[0], buffer, sizeof(buffer) - 1);
	EXPECT_FALSE(ch->isTopicProtected());

	// Note: Removing key (-k) and limit (-l) are not currently implemented
	// The MODE handler doesn't check currentSign for KEY and LIMIT modes
	// So we skip testing -k and -l removal

	close(sv1[0]);
	close(sv1[1]);
}

// Test JOIN with multiple channels
// TEST(ChannelsClientsManagerTest, JoinMultipleChannels) {
// 	int sv[2];
// 	setSocketPair(sv);
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;
// 	Client* client_1 = new Client(sv[1]);
// 	client_1->addToBuffer("PASS correct_password\r\nNICK testuser\r\nUSER testuser 0 * :Real Name\r\n");
// 	clients_map[sv[1]] = client_1;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0);
// 	EXPECT_EQ(client_1->isRegistered(), true);

// 	client_1->clearBuffer();
// 	client_1->addToBuffer("JOIN #channel1,#channel2,#channel3\r\n");
// 	manager.handleClientMessage(client_1);
// 	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0);
// 	std::string output(buffer);
// 	EXPECT_NE(output.find("Welcome to #channel1"), std::string::npos);
// 	EXPECT_NE(output.find("Welcome to #channel2"), std::string::npos);
// 	EXPECT_NE(output.find("Welcome to #channel3"), std::string::npos);

// 	// Verify all channels were created
// 	Channel* ch1 = manager.getChannel("#channel1");
// 	Channel* ch2 = manager.getChannel("#channel2");
// 	Channel* ch3 = manager.getChannel("#channel3");
// 	EXPECT_TRUE(ch1 != nullptr);
// 	EXPECT_TRUE(ch2 != nullptr);
// 	EXPECT_TRUE(ch3 != nullptr);
// 	EXPECT_TRUE(ch1->isClientInChannel(client_1));
// 	EXPECT_TRUE(ch2->isClientInChannel(client_1));
// 	EXPECT_TRUE(ch3->isClientInChannel(client_1));

// 	close(sv[0]);
// 	close(sv[1]);
// }

TEST(ChannelsClientsManagerTest, AllModesTest) {
	std::string correctPass = "correct_password";
	std::vector<pollfd> pollfds;
	std::map<int, Client*> clients_map;
	pollfd server_pfd;
	pollfds.push_back(server_pfd);

	std::vector<Client*> clients;
	// std::vector<int[2]> svs;
	std::vector<std::array<int,2> > svs;

	ChannelsClientsManager manager(clients_map, correctPass, pollfds);
	for (int i = 0; i < 5; ++i) {
		int sv[2];
		Client* client = returnReadyToConnectClient(pollfds, clients_map, sv, correctPass, "testuser" + std::to_string(i), "testuser" + std::to_string(i));
		clients.push_back(client);
		// svs.push_back(sv);
		svs.push_back(std::array<int,2>{sv[0], sv[1]});
	}
	for (size_t i = 0; i < clients.size(); ++i) {
		manager.handleClientMessage(clients[i]);
		char buffer[2048] = {0};
		// Drain any registration/welcome messages
		recv_nonblocking(svs[i][0], buffer, sizeof(buffer) - 1);
		ASSERT_EQ(clients[i]->isRegistered(), true);
		clients[i]->clearBuffer();
	}
	// Joing a channel
	for (size_t i = 0; i < clients.size(); ++i) {
		clients[i]->clearBuffer();
		clients[i]->addToBuffer("JOIN #modechannel\r\n");
		manager.handleClientMessage(clients[i]);
		char buffer[2048] = {0};
		recv_nonblocking(svs[i][0], buffer, sizeof(buffer) - 1);
		ASSERT_NE(std::string(buffer).find("Welcome to #modechannel channel!"), std::string::npos);
		clients[i]->clearBuffer();
	}
	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> clean all the read pipes
	for (size_t i = 0; i < clients.size(); ++i) {
		char buffer[2048] = {0};
		recv_nonblocking(svs[i][0], buffer, sizeof(buffer) - 1);
	}
	// client # 1 is not a moderator
	clients[1]->addToBuffer("MODE #modechannel +i\r\n");
	manager.handleClientMessage(clients[1]);

	for (size_t i = 0; i < clients.size(); ++i) {
		char buffer[2048] = {0};

		ssize_t n = recv_nonblocking(svs[i][0], buffer, sizeof(buffer) - 1);
		if (i == 1) {
			EXPECT_GT(n, 0);
			std::string output = buffer;
			EXPECT_EQ(output, ":ft_irc.42.de 482 #modechannel :You're not channel operator\r\n");
		} else {
			EXPECT_EQ(n, 0);
		}
		clients[i]->clearBuffer();
	}
	EXPECT_EQ(manager.getChannel("#modechannel")->isInviteOnly(), false);
	// client #0 is a moderator
	clients[0]->addToBuffer("MODE #modechannel +i\r\n");
	manager.handleClientMessage(clients[0]);
	for (size_t i = 0; i < clients.size(); ++i) {
		char buffer[2048] = {0};
		ssize_t n = recv_nonblocking(svs[i][0], buffer, sizeof(buffer) - 1);
		if (n > 0) {
			std::string output = buffer;
			EXPECT_EQ(output, ":ft_irc.42.de 324 testuser0 #modechannel :+i\r\n");
		} else {
			EXPECT_EQ(n, 0);
		}
		clients[i]->clearBuffer();
	}
	EXPECT_EQ(manager.getChannel("#modechannel")->isInviteOnly(), true);
	EXPECT_EQ(manager.getChannel("#modechannel")->getOperators().size(), 1);
	// clien # 0 makes client # 1 operator
	clients[0]->addToBuffer("MODE #modechannel +o testuser1\r\n");
	manager.handleClientMessage(clients[0]);
	EXPECT_EQ(manager.getChannel("#modechannel")->getOperators().size(), 2);
	for (size_t i = 0; i < clients.size(); ++i) {
		char buffer[2048] = {0};
		ssize_t n = recv_nonblocking(svs[i][0], buffer, sizeof(buffer) - 1);
		if (n > 0) {
			std::string output = buffer;
			EXPECT_EQ(output, ":ft_irc.42.de 324 testuser0 #modechannel :+o testuser1\r\n");
		} else {
			EXPECT_EQ(n, 0);
		}
		clients[i]->clearBuffer();
	}

	// // now client #1 is operator, add invite-only mode
	EXPECT_EQ(manager.getChannel("#modechannel")->isInviteOnly(), true);
	clients[1]->addToBuffer("MODE #modechannel -i\r\n");
	manager.handleClientMessage(clients[1]);
	EXPECT_EQ(manager.getChannel("#modechannel")->isInviteOnly(), false);
}

// // Test JOIN with invalid channel name
// TEST(ChannelsClientsManagerTest, JoinInvalidChannelName) {
// 	int sv[2];
// 	setSocketPair(sv);
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;
// 	Client* client_1 = new Client(sv[1]);
// 	client_1->addToBuffer("PASS correct_password\r\nNICK testuser\r\nUSER testuser 0 * :Real Name\r\n");
// 	clients_map[sv[1]] = client_1;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	client_1->clearBuffer();
// 	client_1->addToBuffer("JOIN invalidchannel\r\n");
// 	manager.handleClientMessage(client_1);
// 	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0);
// 	std::string output(buffer);
// 	EXPECT_NE(output.find("403"), std::string::npos); // ERR_NOSUCHCHANNEL

// 	close(sv[0]);
// 	close(sv[1]);
// }

// // Test joining the same channel twice
// TEST(ChannelsClientsManagerTest, JoinSameChannelTwice) {
// 	int sv[2];
// 	setSocketPair(sv);
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;
// 	Client* client_1 = new Client(sv[1]);
// 	client_1->addToBuffer("PASS correct_password\r\nNICK testuser\r\nUSER testuser 0 * :Real Name\r\n");
// 	clients_map[sv[1]] = client_1;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	client_1->clearBuffer();
// 	client_1->addToBuffer("JOIN #testchannel\r\n");
// 	manager.handleClientMessage(client_1);
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	// Try joining again
// 	client_1->clearBuffer();
// 	client_1->addToBuffer("JOIN #testchannel\r\n");
// 	manager.handleClientMessage(client_1);
// 	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0);
// 	std::string output(buffer);
// 	EXPECT_NE(output.find("already in this channel"), std::string::npos);

// 	close(sv[0]);
// 	close(sv[1]);
// }

// // PRIVMSG Tests
// TEST(ChannelsClientsManagerTest, PrivmsgToChannel) {
// 	int sv[2], sv2[2];
// 	setSocketPair(sv);
// 	setSocketPair(sv2);
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;

// 	// Register first client
// 	Client* client_1 = new Client(sv[1]);
// 	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\n");
// 	clients_map[sv[1]] = client_1;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	// Register second client
// 	Client* client_2 = new Client(sv2[1]);
// 	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\n");
// 	clients_map[sv2[1]] = client_2;
// 	manager.handleClientMessage(client_2);
// 	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

// 	// Both join channel
// 	client_1->clearBuffer();
// 	client_1->addToBuffer("JOIN #testchannel\r\n");
// 	manager.handleClientMessage(client_1);
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	client_2->clearBuffer();
// 	client_2->addToBuffer("JOIN #testchannel\r\n");
// 	manager.handleClientMessage(client_2);
// 	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1); // Clear user1's buffer from user2 joining

// 	// Send message from user1
// 	client_1->clearBuffer();
// 	client_1->addToBuffer("PRIVMSG #testchannel :Hello everyone!\r\n");
// 	manager.handleClientMessage(client_1);

// 	// Check user2 received the message
// 	ssize_t n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0);
// 	std::string output(buffer);
// 	EXPECT_NE(output.find("user1"), std::string::npos);
// 	EXPECT_NE(output.find("PRIVMSG #testchannel"), std::string::npos);
// 	EXPECT_NE(output.find("Hello everyone!"), std::string::npos);

// 	close(sv[0]);
// 	close(sv[1]);
// 	close(sv2[0]);
// 	close(sv2[1]);
// }

// TEST(ChannelsClientsManagerTest, PrivmsgToUser) {
// 	int sv[2], sv2[2];
// 	setSocketPair(sv);
// 	setSocketPair(sv2);
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;

// 	// Register both clients
// 	Client* client_1 = new Client(sv[1]);
// 	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\n");
// 	clients_map[sv[1]] = client_1;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	Client* client_2 = new Client(sv2[1]);
// 	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\n");
// 	clients_map[sv2[1]] = client_2;
// 	manager.handleClientMessage(client_2);
// 	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

// 	// Send private message from user1 to user2
// 	client_1->clearBuffer();
// 	client_1->addToBuffer("PRIVMSG user2 :Hello user2!\r\n");
// 	manager.handleClientMessage(client_1);

// 	// Check user2 received the message
// 	ssize_t n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0);
// 	std::string output(buffer);
// 	EXPECT_NE(output.find("user1"), std::string::npos);
// 	EXPECT_NE(output.find("PRIVMSG user2"), std::string::npos);
// 	EXPECT_NE(output.find("Hello user2!"), std::string::npos);

// 	close(sv[0]);
// 	close(sv[1]);
// 	close(sv2[0]);
// 	close(sv2[1]);
// }

// TEST(ChannelsClientsManagerTest, PrivmsgToNonExistentUser) {
// 	int sv[2];
// 	setSocketPair(sv);
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;
// 	Client* client_1 = new Client(sv[1]);
// 	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\n");
// 	clients_map[sv[1]] = client_1;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	client_1->clearBuffer();
// 	client_1->addToBuffer("PRIVMSG nonexistent :Hello\r\n");
// 	manager.handleClientMessage(client_1);
// 	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0);
// 	std::string output(buffer);
// 	EXPECT_NE(output.find("401"), std::string::npos); // ERR_NOSUCHNICK

// 	close(sv[0]);
// 	close(sv[1]);
// }

// // TOPIC Tests
// TEST(ChannelsClientsManagerTest, TopicViewAndSet) {
// 	int sv[2], sv2[2];
// 	setSocketPair(sv);
// 	setSocketPair(sv2);
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);

// 	// Register first client
// 	Client* client_1 = new Client(sv[1]);
// 	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\n");
// 	clients_map[sv[1]] = client_1;
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	// Register second client
// 	Client* client_2 = new Client(sv2[1]);
// 	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\n");
// 	clients_map[sv2[1]] = client_2;
// 	manager.handleClientMessage(client_2);
// 	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

// 	// Both join channel
// 	client_1->clearBuffer();
// 	client_1->addToBuffer("JOIN #testchannel\r\n");
// 	manager.handleClientMessage(client_1);
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	client_2->clearBuffer();
// 	client_2->addToBuffer("JOIN #testchannel\r\n");
// 	manager.handleClientMessage(client_2);
// 	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1); // Clear user1's buffer from user2 joining

// 	// View topic (should be not set)
// 	client_1->clearBuffer();
// 	client_1->addToBuffer("TOPIC #testchannel\r\n");
// 	manager.handleClientMessage(client_1);
// 	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0);
// 	std::string output(buffer);
// 	EXPECT_NE(output.find("not set yet"), std::string::npos);

// 	// Set topic (user1 is operator as first user)
// 	client_1->clearBuffer();
// 	client_1->addToBuffer("TOPIC #testchannel :New channel topic\r\n");
// 	manager.handleClientMessage(client_1);

// 	// Check that user2 received the topic change notification
// 	n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0);
// 	output = buffer;
// 	EXPECT_NE(output.find("TOPIC #testchannel"), std::string::npos);
// 	EXPECT_NE(output.find("New channel topic"), std::string::npos);

// 	// View topic again from user1
// 	client_1->clearBuffer();
// 	client_1->addToBuffer("TOPIC #testchannel\r\n");
// 	manager.handleClientMessage(client_1);
// 	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0);
// 	output = buffer;
// 	EXPECT_NE(output.find("New channel topic"), std::string::npos);

// 	close(sv[0]);
// 	close(sv[1]);
// 	close(sv2[0]);
// 	close(sv2[1]);
// }

// TEST(ChannelsClientsManagerTest, TopicSetWithoutOperator) {
// 	int sv[2], sv2[2];
// 	setSocketPair(sv);
// 	setSocketPair(sv2);
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);

// 	// Register and join first client (will be operator)
// 	Client* client_1 = new Client(sv[1]);
// 	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\nJOIN #testchannel\r\n");
// 	clients_map[sv[1]] = client_1;
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	// Register and join second client (not operator)
// 	Client* client_2 = new Client(sv2[1]);
// 	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\nJOIN #testchannel\r\n");
// 	clients_map[sv2[1]] = client_2;
// 	manager.handleClientMessage(client_2);
// 	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

// 	// Try to set topic as non-operator
// 	client_2->clearBuffer();
// 	client_2->addToBuffer("TOPIC #testchannel :Unauthorized topic\r\n");
// 	manager.handleClientMessage(client_2);
// 	ssize_t n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0);
// 	std::string output(buffer);
// 	EXPECT_NE(output.find("operator rights"), std::string::npos);

// 	close(sv[0]);
// 	close(sv[1]);
// 	close(sv2[0]);
// 	close(sv2[1]);
// }

// // KICK Tests
// TEST(ChannelsClientsManagerTest, KickUser) {
// 	int sv[2], sv2[2];
// 	setSocketPair(sv);
// 	setSocketPair(sv2);
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);

// 	// Register both clients and join channel
// 	Client* client_1 = new Client(sv[1]);
// 	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\nJOIN #testchannel\r\n");
// 	clients_map[sv[1]] = client_1;
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	Client* client_2 = new Client(sv2[1]);
// 	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\nJOIN #testchannel\r\n");
// 	clients_map[sv2[1]] = client_2;
// 	manager.handleClientMessage(client_2);
// 	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1); // Clear user1's buffer

// 	// Kick user2
// 	client_1->clearBuffer();
// 	client_1->addToBuffer("KICK #testchannel user2 :Bad behavior\r\n");
// 	manager.handleClientMessage(client_1);

// 	// Check user2 was kicked
// 	ssize_t n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0);
// 	std::string output(buffer);
// 	EXPECT_NE(output.find("kicked"), std::string::npos);
// 	EXPECT_NE(output.find("Bad behavior"), std::string::npos);

// 	// Verify user2 is no longer in channel
// 	Channel* ch = manager.getChannel("#testchannel");
// 	EXPECT_FALSE(ch->isClientInChannel(client_2));

// 	close(sv[0]);
// 	close(sv[1]);
// 	close(sv2[0]);
// 	close(sv2[1]);
// }

// TEST(ChannelsClientsManagerTest, KickWithoutOperator) {
// 	int sv[2], sv2[2];
// 	setSocketPair(sv);
// 	setSocketPair(sv2);
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);

// 	Client* client_1 = new Client(sv[1]);
// 	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\nJOIN #testchannel\r\n");
// 	clients_map[sv[1]] = client_1;
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	Client* client_2 = new Client(sv2[1]);
// 	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\nJOIN #testchannel\r\n");
// 	clients_map[sv2[1]] = client_2;
// 	manager.handleClientMessage(client_2);
// 	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

// 	// Try to kick as non-operator
// 	client_2->clearBuffer();
// 	client_2->addToBuffer("KICK #testchannel user1 :Unauthorized kick\r\n");
// 	manager.handleClientMessage(client_2);
// 	ssize_t n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0);
// 	std::string output(buffer);
// 	EXPECT_NE(output.find("482"), std::string::npos); // ERR_CHANOPRIVSNEEDED

// 	close(sv[0]);
// 	close(sv[1]);
// 	close(sv2[0]);
// 	close(sv2[1]);
// }

// // INVITE Tests
// TEST(ChannelsClientsManagerTest, InviteUser) {
// 	int sv[2], sv2[2];
// 	setSocketPair(sv);
// 	setSocketPair(sv2);
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);

// 	// Register both clients
// 	Client* client_1 = new Client(sv[1]);
// 	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\nJOIN #testchannel\r\n");
// 	clients_map[sv[1]] = client_1;
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	Client* client_2 = new Client(sv2[1]);
// 	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\n");
// 	clients_map[sv2[1]] = client_2;
// 	manager.handleClientMessage(client_2);
// 	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

// 	// Invite user2 to channel
// 	client_1->clearBuffer();
// 	client_1->addToBuffer("INVITE user2 #testchannel\r\n");
// 	manager.handleClientMessage(client_1);

// 	// Check user2 received invite
// 	ssize_t n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0);
// 	std::string output(buffer);
// 	EXPECT_NE(output.find("invited"), std::string::npos);
// 	EXPECT_NE(output.find("#testchannel"), std::string::npos);

// 	// Check user1 received confirmation
// 	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0);
// 	output = buffer;
// 	EXPECT_NE(output.find("successfully invited"), std::string::npos);

// 	close(sv[0]);
// 	close(sv[1]);
// 	close(sv2[0]);
// 	close(sv2[1]);
// }

// TEST(ChannelsClientsManagerTest, InviteToNonExistentChannel) {
// 	int sv[2], sv2[2];
// 	setSocketPair(sv);
// 	setSocketPair(sv2);
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);

// 	Client* client_1 = new Client(sv[1]);
// 	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\n");
// 	clients_map[sv[1]] = client_1;
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	Client* client_2 = new Client(sv2[1]);
// 	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\n");
// 	clients_map[sv2[1]] = client_2;
// 	manager.handleClientMessage(client_2);
// 	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

// 	// Try to invite to non-existent channel
// 	client_1->clearBuffer();
// 	client_1->addToBuffer("INVITE user2 #nonexistent\r\n");
// 	manager.handleClientMessage(client_1);
// 	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0);
// 	std::string output(buffer);
// 	EXPECT_NE(output.find("403"), std::string::npos); // Channel doesn't exist

// 	close(sv[0]);
// 	close(sv[1]);
// 	close(sv2[0]);
// 	close(sv2[1]);
// }

// // Complex/Edge Case Tests

// // Test sending message to non-existent channel
// TEST(ChannelsClientsManagerTest, PrivmsgToNonExistentChannel) {
// 	int sv[2];
// 	setSocketPair(sv);
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);

// 	Client* client_1 = new Client(sv[1]);
// 	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\n");
// 	clients_map[sv[1]] = client_1;
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	// Try to send message to non-existent channel
// 	client_1->clearBuffer();
// 	client_1->addToBuffer("PRIVMSG #nonexistent :Hello nobody!\r\n");
// 	manager.handleClientMessage(client_1);
// 	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0);
// 	std::string output(buffer);
// 	EXPECT_NE(output.find("403"), std::string::npos); // Channel doesn't exist

// 	close(sv[0]);
// 	close(sv[1]);
// }

// // Test sending message to channel you're not a member of
// TEST(ChannelsClientsManagerTest, PrivmsgToChannelNotMember) {
// 	int sv[2], sv2[2];
// 	setSocketPair(sv);
// 	setSocketPair(sv2);
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);

// 	// Register and join user1
// 	Client* client_1 = new Client(sv[1]);
// 	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\nJOIN #private\r\n");
// 	clients_map[sv[1]] = client_1;
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	// Register user2 but don't join
// 	Client* client_2 = new Client(sv2[1]);
// 	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\n");
// 	clients_map[sv2[1]] = client_2;
// 	manager.handleClientMessage(client_2);
// 	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

// 	// Try to send message to channel user2 is not in
// 	client_2->clearBuffer();
// 	client_2->addToBuffer("PRIVMSG #private :I shouldn't be able to send this!\r\n");
// 	manager.handleClientMessage(client_2);
// 	ssize_t n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0);
// 	std::string output(buffer);
// 	EXPECT_NE(output.find("access"), std::string::npos);

// 	close(sv[0]);
// 	close(sv[1]);
// 	close(sv2[0]);
// 	close(sv2[1]);
// }

// // Test sending message after being kicked
// TEST(ChannelsClientsManagerTest, PrivmsgAfterBeingKicked) {
// 	int sv[2], sv2[2];
// 	setSocketPair(sv);
// 	setSocketPair(sv2);
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);

// 	// Register both clients and join channel
// 	Client* client_1 = new Client(sv[1]);
// 	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\nJOIN #testchannel\r\n");
// 	clients_map[sv[1]] = client_1;
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	Client* client_2 = new Client(sv2[1]);
// 	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\nJOIN #testchannel\r\n");
// 	clients_map[sv2[1]] = client_2;
// 	manager.handleClientMessage(client_2);
// 	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	// Kick user2
// 	client_1->clearBuffer();
// 	client_1->addToBuffer("KICK #testchannel user2 :You're out!\r\n");
// 	manager.handleClientMessage(client_1);
// 	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	// Try to send message after being kicked
// 	client_2->clearBuffer();
// 	client_2->addToBuffer("PRIVMSG #testchannel :But I want to talk!\r\n");
// 	manager.handleClientMessage(client_2);
// 	ssize_t n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0);
// 	std::string output(buffer);
// 	EXPECT_NE(output.find("access"), std::string::npos);

// 	// Verify user1 didn't receive the message
// 	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_EQ(n, 0); // No message should be received

// 	close(sv[0]);
// 	close(sv[1]);
// 	close(sv2[0]);
// 	close(sv2[1]);
// }

// // Test inviting user who is already in channel
// TEST(ChannelsClientsManagerTest, InviteUserAlreadyInChannel) {
// 	int sv[2], sv2[2];
// 	setSocketPair(sv);
// 	setSocketPair(sv2);
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);

// 	// Register and join both clients
// 	Client* client_1 = new Client(sv[1]);
// 	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\nJOIN #testchannel\r\n");
// 	clients_map[sv[1]] = client_1;
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	Client* client_2 = new Client(sv2[1]);
// 	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\nJOIN #testchannel\r\n");
// 	clients_map[sv2[1]] = client_2;
// 	manager.handleClientMessage(client_2);
// 	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	// Try to invite user2 who is already in the channel
// 	client_1->clearBuffer();
// 	client_1->addToBuffer("INVITE user2 #testchannel\r\n");
// 	manager.handleClientMessage(client_1);
// 	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0);
// 	std::string output(buffer);
// 	EXPECT_NE(output.find("already"), std::string::npos);

// 	close(sv[0]);
// 	close(sv[1]);
// 	close(sv2[0]);
// 	close(sv2[1]);
// }

// // Test kicking user who is not in channel
// TEST(ChannelsClientsManagerTest, KickUserNotInChannel) {
// 	int sv[2], sv2[2];
// 	setSocketPair(sv);
// 	setSocketPair(sv2);
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);

// 	// Register user1 and join channel
// 	Client* client_1 = new Client(sv[1]);
// 	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\nJOIN #testchannel\r\n");
// 	clients_map[sv[1]] = client_1;
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	// Register user2 but don't join
// 	Client* client_2 = new Client(sv2[1]);
// 	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\n");
// 	clients_map[sv2[1]] = client_2;
// 	manager.handleClientMessage(client_2);
// 	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

// 	// Try to kick user2 who is not in channel
// 	client_1->clearBuffer();
// 	client_1->addToBuffer("KICK #testchannel user2 :Get out!\r\n");
// 	manager.handleClientMessage(client_1);
// 	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0);
// 	std::string output(buffer);
// 	EXPECT_NE(output.find("441"), std::string::npos); // ERR_USERNOTINCHANNEL

// 	close(sv[0]);
// 	close(sv[1]);
// 	close(sv2[0]);
// 	close(sv2[1]);
// }

// // Test setting topic on non-existent channel
// TEST(ChannelsClientsManagerTest, TopicOnNonExistentChannel) {
// 	int sv[2];
// 	setSocketPair(sv);
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);

// 	Client* client_1 = new Client(sv[1]);
// 	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\n");
// 	clients_map[sv[1]] = client_1;
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	// Try to set topic on non-existent channel
// 	client_1->clearBuffer();
// 	client_1->addToBuffer("TOPIC #nonexistent :New topic\r\n");
// 	manager.handleClientMessage(client_1);
// 	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0);
// 	std::string output(buffer);
// 	EXPECT_NE(output.find("doesn't exist"), std::string::npos);

// 	close(sv[0]);
// 	close(sv[1]);
// }

// // Test kick, rejoin, and verify operator status is lost
// TEST(ChannelsClientsManagerTest, KickRejoinLoseOperator) {
// 	int sv[2], sv2[2];
// 	setSocketPair(sv);
// 	setSocketPair(sv2);
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);

// 	// Register and join both clients
// 	Client* client_1 = new Client(sv[1]);
// 	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\nJOIN #testchannel\r\n");
// 	clients_map[sv[1]] = client_1;
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	Client* client_2 = new Client(sv2[1]);
// 	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\nJOIN #testchannel\r\n");
// 	clients_map[sv2[1]] = client_2;
// 	manager.handleClientMessage(client_2);
// 	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	// Verify initial operator status
// 	Channel* ch = manager.getChannel("#testchannel");
// 	EXPECT_TRUE(ch->isOperator(client_1));
// 	EXPECT_FALSE(ch->isOperator(client_2));

// 	// Kick user2
// 	client_1->clearBuffer();
// 	client_1->addToBuffer("KICK #testchannel user2 :Kicked\r\n");
// 	manager.handleClientMessage(client_1);
// 	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	// User2 rejoins
// 	client_2->clearBuffer();
// 	client_2->addToBuffer("JOIN #testchannel\r\n");
// 	manager.handleClientMessage(client_2);
// 	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	// Verify user2 is back but not operator
// 	EXPECT_TRUE(ch->isClientInChannel(client_2));
// 	EXPECT_FALSE(ch->isOperator(client_2));

// 	// User2 should not be able to kick user1
// 	client_2->clearBuffer();
// 	client_2->addToBuffer("KICK #testchannel user1 :Revenge!\r\n");
// 	manager.handleClientMessage(client_2);
// 	ssize_t n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0);
// 	std::string output(buffer);
// 	EXPECT_NE(output.find("482"), std::string::npos); // ERR_CHANOPRIVSNEEDED

// 	close(sv[0]);
// 	close(sv[1]);
// 	close(sv2[0]);
// 	close(sv2[1]);
// }

// // Test inviting non-existent user
// TEST(ChannelsClientsManagerTest, InviteNonExistentUser) {
// 	int sv[2];
// 	setSocketPair(sv);
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);

// 	Client* client_1 = new Client(sv[1]);
// 	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\nJOIN #testchannel\r\n");
// 	clients_map[sv[1]] = client_1;
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	// Try to invite non-existent user
// 	client_1->clearBuffer();
// 	client_1->addToBuffer("INVITE ghostuser #testchannel\r\n");
// 	manager.handleClientMessage(client_1);
// 	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0);
// 	std::string output(buffer);
// 	EXPECT_NE(output.find("401"), std::string::npos); // ERR_NOSUCHNICK

// 	close(sv[0]);
// 	close(sv[1]);
// }

// // Test multiple users sending messages simultaneously
// TEST(ChannelsClientsManagerTest, MultipleUsersMessaging) {
// 	int sv[2], sv2[2], sv3[2];
// 	setSocketPair(sv);
// 	setSocketPair(sv2);
// 	setSocketPair(sv3);
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);

// 	// Register three clients
// 	Client* client_1 = new Client(sv[1]);
// 	client_1->addToBuffer("PASS correct_password\r\nNICK user1\r\nUSER user1 0 * :User One\r\nJOIN #chat\r\n");
// 	clients_map[sv[1]] = client_1;
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	Client* client_2 = new Client(sv2[1]);
// 	client_2->addToBuffer("PASS correct_password\r\nNICK user2\r\nUSER user2 0 * :User Two\r\nJOIN #chat\r\n");
// 	clients_map[sv2[1]] = client_2;
// 	manager.handleClientMessage(client_2);
// 	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);

// 	Client* client_3 = new Client(sv3[1]);
// 	client_3->addToBuffer("PASS correct_password\r\nNICK user3\r\nUSER user3 0 * :User Three\r\nJOIN #chat\r\n");
// 	clients_map[sv3[1]] = client_3;
// 	manager.handleClientMessage(client_3);
// 	recv_nonblocking(sv3[0], buffer, sizeof(buffer) - 1);
// 	recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);

// 	// User1 sends message
// 	client_1->clearBuffer();
// 	client_1->addToBuffer("PRIVMSG #chat :Hello from user1\r\n");
// 	manager.handleClientMessage(client_1);

// 	// Verify user2 and user3 received it
// 	ssize_t n2 = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n2, 0);
// 	std::string output2(buffer);
// 	EXPECT_NE(output2.find("Hello from user1"), std::string::npos);

// 	ssize_t n3 = recv_nonblocking(sv3[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n3, 0);
// 	std::string output3(buffer);
// 	EXPECT_NE(output3.find("Hello from user1"), std::string::npos);

// 	// User2 replies
// 	client_2->clearBuffer();
// 	client_2->addToBuffer("PRIVMSG #chat :Hi user1!\r\n");
// 	manager.handleClientMessage(client_2);

// 	// Verify user1 and user3 received it
// 	ssize_t n1 = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n1, 0);
// 	std::string output1(buffer);
// 	EXPECT_NE(output1.find("Hi user1!"), std::string::npos);

// 	n3 = recv_nonblocking(sv3[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n3, 0);
// 	output3 = buffer;
// 	EXPECT_NE(output3.find("Hi user1!"), std::string::npos);

// 	close(sv[0]);
// 	close(sv[1]);
// 	close(sv2[0]);
// 	close(sv2[1]);
// 	close(sv3[0]);
// 	close(sv3[1]);
// }

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}




// MESSAGE: MODE Welcome to the ft_IRC Network +i
// :ft_irc.42.de 461 <nick> MODE :Not enough parameters
// MESSAGE: WHOIS zarina
// :ft_irc.42.de 311 <requester> zarina user host * :Real Name
// :ft_irc.42.de 312 <requester> zarina ft_irc.42.de :Server info
// :ft_irc.42.de 318 <requester> zarina :End of WHOIS list

// MESSAGE: MODE zarina +i
// :ft_irc.42.de MODE zarina +i
// MESSAGE: PING ft_irc.42.de
