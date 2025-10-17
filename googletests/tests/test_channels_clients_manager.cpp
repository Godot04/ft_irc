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


// TEST(ChannelsClientsManagerTest, JoinWithoutRegistering) {
// 	int sv[2];
// 	ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0); // sv[0] and sv[1] are connected sockets
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;
// 	Client* client_1 = new Client(sv[1]); // use one end for the client
// 	client_1->addToBuffer("PASS wrong_password\r\n"); // Make sure to use CRLF for IRC
// 	clients_map[1] = client_1;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	close(sv[1]); // Close the write end after sending
// 	ssize_t n = read(sv[0], buffer, sizeof(buffer) - 1);
// 	ASSERT_GT(n, 0); // Ensure something was written
// 	std::string output(buffer);
// 	EXPECT_EQ(client_1->isAuthenticated(), false);
// 	EXPECT_NE(output.find("Password incorrect."), std::string::npos);
// 	close(sv[0]);
// }

// TEST(ChannelsClientsManagerTest, JoinWithCorrectPassword) {
// 	int sv[2];
// 	setSocketPair(sv);
// 	// ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0); // sv[0] and sv[1] are connected sockets
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;
// 	Client* client_1 = new Client(sv[1]); // use one end for the client
// 	client_1->addToBuffer("PASS correct_password\r\n"); // Make sure to use CRLF for IRC
// 	clients_map[sv[1]] = client_1;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_EQ(n, 0); // Ensure something was written
// 	std::string output(buffer);
// 	EXPECT_NE(output.find(""), std::string::npos);
// 	EXPECT_EQ(client_1->isAuthenticated(), true);
// 	EXPECT_EQ(client_1->isNicknameSet(), false);
// 	client_1->addToBuffer("NICK testuser\r\n");
// 	manager.handleClientMessage(client_1);
// 	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_EQ(n, 0); // Ensure something was written
// 	std::string output2(buffer);
// 	EXPECT_NE(output2.find(""), std::string::npos);
// 	EXPECT_EQ(client_1->isNicknameSet(), true);
// 	EXPECT_EQ(client_1->getNickname(), "testuser");
// 	EXPECT_EQ(client_1->isRegistered(), false);

// 	// testing duplicate nick
// 	int sv2[2];
// 	setSocketPair(sv2);
// 	Client* client_2 = new Client(sv2[1]); // use one end for the client
// 	client_2->addToBuffer("PASS correct_password\r\nNICK testuser\r\n");
// 	clients_map[sv2[1]] = client_2;
// 	manager.handleClientMessage(client_2);
// 	n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0); // Ensure something was written
// 	std::string output3 = buffer;
// 	EXPECT_EQ(output3, ":ft_irc.42.de 433 * :testuser Nickname is already in use\r\n");
// 	EXPECT_EQ(client_2->isNicknameSet(), false);
// 	EXPECT_EQ(client_2->isAuthenticated(), true);
// 	EXPECT_EQ(client_2->isRegistered(), false);
// 	client_2->clearBuffer();
// 	client_2->addToBuffer("NICK testuser2\r\n");
// 	manager.handleClientMessage(client_2);
// 	n = recv_nonblocking(sv2[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_EQ(n, 0); // Ensure something was written
// 	std::string output4 = buffer;
// 	EXPECT_NE(output4.find(""), std::string::npos);
// 	EXPECT_EQ(client_2->isNicknameSet(), true);
// 	EXPECT_EQ(client_2->getNickname(), "testuser2");
// 	EXPECT_EQ(client_2->isRegistered(), false);


// 	// Now send USER command to complete registration
// 	client_1->addToBuffer("USER testuser 0 * :Real Name\r\n");
// 	manager.handleClientMessage(client_1);
// 	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0); // Ensure something was written
// 	std::string output5 = buffer;
// 	// EXPECT_EQ(output5, ":ft_irc.42.de 001 testuser :Welcome to the ft_IRC Network, testuser\r\n");
// 	EXPECT_EQ(client_1->isRegistered(), true);

// 	close(sv[0]);
// 	close(sv[1]);
// }

// // join channels

// // test CAP LS / ACK negotiation
// TEST(ChannelsClientsManagerTest, CapLsAckNegotiation) {
// 	int sv[2];
// 	setSocketPair(sv);
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;
// 	Client* client_1 = new Client(sv[1]); // use one end for the client
// 	client_1->addToBuffer("PASS correct_password\r\nCAP LS\r\nNICK testuser\r\nUSER testuser 0 * :Real Name\r\n");
// 	clients_map[sv[1]] = client_1;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0); // Ensure something was written
// 	std::string output(buffer);
// 	EXPECT_NE(output.find(""), std::string::npos);
// 	EXPECT_EQ(client_1->isRegistered(), false);
// 	client_1->clearBuffer();
// 	client_1->addToBuffer("CAP END\r\n");
// 	manager.handleClientMessage(client_1);
// 	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0); // Ensure something was written
// 	std::string output2(buffer);
// 	EXPECT_EQ(output2, ":ft_irc.42.de 001 testuser :Welcome to the ft_IRC Network\r\n");
// 	EXPECT_EQ(client_1->isRegistered(), true);
// 	close(sv[0]);
// 	close(sv[1]);
// }


// TEST(ChannelsClientsManagerTest, PassUserAfterRegisering) {
// 	int sv[2];
// 	setSocketPair(sv);
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;
// 	Client* client_1 = new Client(sv[1]); // use one end for the client
// 	client_1->addToBuffer("PASS correct_password\r\nNICK testuser\r\nUSER testuser 0 * :Real Name\r\n");
// 	clients_map[sv[1]] = client_1;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0); // Ensure something was written
// 	std::string output(buffer);
// 	EXPECT_NE(output.find(""), std::string::npos);
// 	EXPECT_EQ(client_1->isRegistered(), true);
// 	client_1->clearBuffer();
// 	client_1->addToBuffer("PASS correct_password\r\n");
// 	manager.handleClientMessage(client_1);
// 	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0); // No response expected
// 	std::string output2(buffer);
// 	EXPECT_EQ(output2, ":ft_irc.42.de 462 testuser :You may not reregister\r\n");
// 	EXPECT_EQ(client_1->isRegistered(), true); // Still registered
// 	// valid User second time
// 	client_1->clearBuffer();
// 	client_1->addToBuffer("USER testuser 0 * :Real Name\r\n");
// 	manager.handleClientMessage(client_1);
// 	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0); // No response expected
// 	std::string output3(buffer);
// 	EXPECT_EQ(output3, ":ft_irc.42.de 462 testuser :You may not reregister\r\n");
// 	EXPECT_EQ(client_1->isRegistered(), true); // Still registered

// 	// password second time
// 	client_1->clearBuffer();
// 	client_1->addToBuffer("PASS correct_password\r\n");
// 	manager.handleClientMessage(client_1);
// 	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0); // No response expected
// 	std::string output4(buffer);
// 	EXPECT_EQ(output4, ":ft_irc.42.de 462 testuser :You may not reregister\r\n");
// 	EXPECT_EQ(client_1->isRegistered(), true); // Still registered

// 	// setting nick second time
// 	client_1->clearBuffer();
// 	client_1->addToBuffer("NICK newnick\r\n");
// 	manager.handleClientMessage(client_1);
// 	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	close(sv[0]);
// 	close(sv[1]);
// }


// // Join a channel after registering
// TEST(ChannelsClientsManagerTest, JoinChannelAfterRegistering) {
// 	int sv[2];
// 	setSocketPair(sv);
// 	ChannelsClientsManager manager;
// 	std::map<int, Client*> clients_map;
// 	Client* client_1 = new Client(sv[1]); // use one end for the client
// 	client_1->addToBuffer("PASS correct_password\r\nNICK testuser\r\nUSER testuser 0 * :Real Name\r\n");
// 	clients_map[sv[1]] = client_1;
// 	std::string password = "correct_password";
// 	manager.setClientsMap(&clients_map, &password, NULL);
// 	manager.handleClientMessage(client_1);
// 	char buffer[1024] = {0};
// 	ssize_t n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0); // Ensure something was written
// 	std::string output(buffer);
// 	EXPECT_NE(output.find(""), std::string::npos);
// 	EXPECT_EQ(client_1->isRegistered(), true);
// 	client_1->clearBuffer();
// 	client_1->addToBuffer("JOIN #testchannel\r\n");
// 	manager.handleClientMessage(client_1);
// 	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
// 	EXPECT_GT(n, 0); // Ensure something was written
// 	std::string output2(buffer);
// 	EXPECT_NE(output2.find("Welcome to #testchannel channel!"), std::string::npos);
// 	Channel* testChannel = manager.getChannel("#testchannel");
// 	bool clientStatus = testChannel->isClientInChannel(client_1);
// 	EXPECT_EQ(clientStatus, true);

// 	// 

// 	close(sv[0]);
// 	close(sv[1]);
// }



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Check Manager Structure <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

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
	n = recv_nonblocking(sv[0], buffer, sizeof(buffer) - 1);
	EXPECT_EQ(0, 0);
	// >>>>>>>>>>>>>>>>>>>>>>>>>>> client2
	int sv2[2];
	Client* client2 = returnReadyToConnectClient(pollfds, clients_map, sv2, correctPass, "testuser2", "testuser2");
	manager.handleClientMessage(client2);
	char buffer2[1024] = {0};
	recv_nonblocking(sv2[0], buffer2, sizeof(buffer2) - 1);
	ASSERT_EQ(client2->isRegistered(), true);

	client2->clearBuffer();
	client2->addToBuffer("JOIN #testchannel\r\n");
	manager.handleClientMessage(client2);
	ssize_t n2 = recv_nonblocking(sv2[0], buffer2, sizeof(buffer2) - 1);
	EXPECT_GT(n2, 0);
	std::string output2 = buffer2;
	EXPECT_EQ(output2, "Welcome to #testchannel channel!\r\nTopic of this channel is not set yet\r\n");
	client->clearBuffer();
	client->addToBuffer("MODE #testchannel +i\r\n");

	manager.handleClientMessage(client);
	ssize_t n3 = recv_nonblocking(sv2[0], buffer2, sizeof(buffer2) - 1);
	EXPECT_GT(n3, 0);
	std::string output3 = buffer2;
	EXPECT_EQ(output3, ":ft_irc.42.de 324 testuser #testchannel :+i\r\n");
	// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


	// >>>>>>>>>>>>>>>>>>>>>>>> topic test
	client->clearBuffer();
	client->addToBuffer("MODE #testchannel +t\r\n");
	manager.handleClientMessage(client);
	ssize_t n4 = recv_nonblocking(sv2[0], buffer2, sizeof(buffer2) - 1);
	EXPECT_GT(n4, 0);
	std::string output4 = buffer2;
	EXPECT_EQ(output4, ":ft_irc.42.de 324 testuser #testchannel :+t\r\n");
	// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	// >>>>>>>>>>>>>>>>>>>>>>>> key test
	client->clearBuffer();
	client->addToBuffer("MODE #testchannel +k secretkey\r\n");
	manager.handleClientMessage(client);
	ssize_t n5 = recv_nonblocking(sv2[0], buffer2, sizeof(buffer2) - 1);
	EXPECT_GT(n5, 0);
	std::string output5 = buffer2;
	EXPECT_EQ(output5, ":ft_irc.42.de 324 testuser #testchannel :+k secretkey\r\n");
	// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	// >>>>>>>>>>>>>>>>>>>>>>>> limit test

	close(sv[0]);
	close(sv[1]);
}

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
