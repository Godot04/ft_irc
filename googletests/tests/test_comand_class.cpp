#include <gtest/gtest.h>
#include "../../inc/Command.hpp"
#include "../../inc/Server.hpp"
#include "../../inc/Client.hpp"
#include "../../inc/IRCCommand.hpp"
#include "../../inc/ReplyNumbers.hpp"

TEST(CommandClassTest, ExecuteMethodTest)
{
	std::string passStr = "PASS 123\r\n";
	IRCCommand cmd(passStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getCommand(), "PASS");
	std::vector<std::string> params = cmd.getParams();
	EXPECT_EQ(params.size(), 1);
	EXPECT_EQ(params.at(0), "123");
	EXPECT_EQ(cmd.getErrorNum(), NO_ERROR);
}

TEST(CommandClassTest, InvalidCommandTest)
{
	std::string passStr = "PASS\r\n";
	IRCCommand cmd(passStr);
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getCommand(), "PASS");
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);
}

TEST(CommandClassTest, ValidCommandWithExtraParamsTest)
{
	std::string passStr = "PASS 123 EXTRA\r\n";
	IRCCommand cmd(passStr);
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getCommand(), "PASS");
	std::vector<std::string> params = cmd.getParams();
	EXPECT_EQ(params.size(), 2);
	EXPECT_EQ(params.at(0), "123");
	EXPECT_EQ(params.at(1), "EXTRA");
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);
}

TEST(CommandClassTest, NoCRLFTest)
{
	std::string passStr = "PASS 123";
	IRCCommand cmd(passStr);
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);

}

TEST(CommandClassTest, EmptyCommandTest)
{
	std::string passStr = "";
	IRCCommand cmd(passStr);
	EXPECT_EQ(cmd.isValid(), false);
}

TEST(CommandClassTest, OnlyCRLFTest) {
	std::string passStr = "\r\n";
	IRCCommand cmd(passStr);
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);
}

TEST(CommandClassTest, NickChangeAgainTest) {
	std::string nickStr = ":WiZ NICK Kilroy\r\n";
	std::string emptyError = "";
	IRCCommand cmd(nickStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getCommand(), "NICK");
	EXPECT_EQ(cmd.getPrefix(), "WiZ");
	std::vector<std::string> params = cmd.getParams();
	EXPECT_EQ(params.size(), 1);
	EXPECT_EQ(params.at(0), "Kilroy");
	EXPECT_EQ(cmd.getErrorNum(), emptyError);
}

TEST(CommandClassTest, InvalidPrefixTest) {
	std::string nickStr = ": NICK Kilroy\r\n";
	IRCCommand cmd(nickStr);
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getCommand(), "NICK");
	EXPECT_EQ(cmd.getPrefix(), "");
	std::vector<std::string> params = cmd.getParams();
	EXPECT_EQ(params.size(), 1);
	EXPECT_EQ(params.at(0), "Kilroy");
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);
}

// MODE Channel modes
// Command: MODE
//  Parameters: <channel> {[+|-]|i|t|k|o|l} [<limit>] [<user>]
            //    [<ban mask>]

// MODE #Finnish +im               ; Makes #Finnish channel moderated and
//                                 'invite-only'.
// e.g.
//            i - invite-only channel flag;
// MODE #Finnish +i

        //    t - topic settable by channel operator only flag;
// MODE #Finnish +t                ; Makes #Finnish channel topic settable by channel operator only.

// k - set a channel key (password).
// MODE #Finnish +k secret         ; Sets the channel key to "secret".

//            o - give/take channel operator privileges;
// MODE #Finnish +o Kilroy         ; Gives 'chanop' privileges to Kilroy on

// l - set the user limit to channel;
// MODE #Finnish +l 10             ; Sets the user limit to 10 on #Finnish.

// MODE #Finnish +lk <limit> <key>

TEST(CommandClassTest, ModeCommandInviteOnlyTest) {
	std::string modeStr = "MODE #Finnish +i\r\n";
	IRCCommand cmd(modeStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getErrorNum(), "");
	EXPECT_EQ(cmd.getCommand(), "MODE");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 2);
	EXPECT_EQ(params.at(0), "#Finnish");
	EXPECT_EQ(params.at(1), "+i");
}

TEST(CommandClassTest, ModeLimitTest) {
	std::string modeStr = "MODE #Finnish +l wrong\r\n";
	IRCCommand cmd(modeStr);
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getCommand(), "MODE");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 3);
	EXPECT_EQ(params.at(0), "#Finnish");
	EXPECT_EQ(params.at(1), "+l");
	EXPECT_EQ(params.at(2), "wrong");
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);
}

TEST(CommandClassTest, ModeCommandSetKeyValidTest) {
	std::string modeStr = "MODE #Finnish +k secret\r\n";
	IRCCommand cmd(modeStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getErrorNum(), "");
	EXPECT_EQ(cmd.getCommand(), "MODE");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 3);
	EXPECT_EQ(params.at(0), "#Finnish");
	EXPECT_EQ(params.at(1), "+k");
	EXPECT_EQ(params.at(2), "secret");
}

TEST(CommandClassTest, ModeCommandSetKeyNoKeyTest) {
	std::string modeStr = "MODE #Finnish +k\r\n";
	IRCCommand cmd(modeStr);
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getCommand(), "MODE");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 2);
	EXPECT_EQ(params.at(0), "#Finnish");
	EXPECT_EQ(params.at(1), "+k");
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);
}

TEST(CommandClassTest, ModeCommandSetLimitAndKeyValidTest) {
	std::string modeStr = "MODE #Finnish +lk 10 secret\r\n";
	IRCCommand cmd(modeStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getErrorNum(), "");
	EXPECT_EQ(cmd.getCommand(), "MODE");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 4);
	EXPECT_EQ(params.at(0), "#Finnish");
	EXPECT_EQ(params.at(1), "+lk");
	EXPECT_EQ(params.at(2), "10");
	EXPECT_EQ(params.at(3), "secret");
}

TEST(CommandClassTest, ModeCommandSetLimitAndKeyMissingKeyTest) {
	std::string modeStr = "MODE #Finnish +lk 10\r\n";
	IRCCommand cmd(modeStr);
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getCommand(), "MODE");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 3);
	EXPECT_EQ(params.at(0), "#Finnish");
	EXPECT_EQ(params.at(1), "+lk");
	EXPECT_EQ(params.at(2), "10");
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);
}

TEST(CommandClassTest, ModeCommandSetLimitAndKeyMissingLimitAndKeyTest) {
	std::string modeStr = "MODE #Finnish +lk\r\n";
	IRCCommand cmd(modeStr);
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getCommand(), "MODE");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 2);
	EXPECT_EQ(params.at(0), "#Finnish");
	EXPECT_EQ(params.at(1), "+lk");
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);
}

TEST(CommandClassTest, ModeCommandSetKeyAndLimitWrongOrderTest) {
	std::string modeStr = "MODE #Finnish +lk secret 10\r\n";
	IRCCommand cmd(modeStr);
	// The correct order for +lk is limit then key, so this should be invalid
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getCommand(), "MODE");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 4);
	EXPECT_EQ(params.at(0), "#Finnish");
	EXPECT_EQ(params.at(1), "+lk");
	EXPECT_EQ(params.at(2), "secret");
	EXPECT_EQ(params.at(3), "10");
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);
}




// TEST(...)
// client
// channel
// JOIN Cann
// EXPECT_EQ(channel, Can)



//       Command: USER
//    Parameters: <username> <hostname> <servername> <realname>
//    USER guest tolmoon tolsun :Ronnie Reagan

TEST(CommandClassTest, UserCommandTest) {
	std::string userStr = "USER guest tolmoon tolsun :Ronnie Reagan\r\n";
	IRCCommand cmd(userStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getCommand(), "USER");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 4);
	EXPECT_EQ(params.at(0), "guest");
	EXPECT_EQ(params.at(1), "tolmoon");
	EXPECT_EQ(params.at(2), "tolsun");
	EXPECT_EQ(params.at(3), ":Ronnie Reagan");
	EXPECT_EQ(cmd.getErrorNum(), "");
}

TEST(CommandClassTest, UserCommandOnlyCRLFTest) {
	std::string userStr = "USER\r\n";
	IRCCommand cmd(userStr);
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getCommand(), "USER");
	std::vector<std::string> params = cmd.getParams();
	EXPECT_EQ(params.size(), 0);
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);
}

TEST(CommandClassTest, UserCommandWithColonButNoRealnameTest) {
	std::string userStr = "USER guest tolmoon tolsun :\r\n";
	IRCCommand cmd(userStr);
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getCommand(), "USER");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 4);
	EXPECT_EQ(params.at(0), "guest");
	EXPECT_EQ(params.at(1), "tolmoon");
	EXPECT_EQ(params.at(2), "tolsun");
	EXPECT_EQ(params.at(3), ":");
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);
}

TEST(CommandClassTest, UserCommandRealnameInMiddleTest) {
	std::string userStr = "USER guest :Ronnie Reagan tolmoon tolsun\r\n";
	IRCCommand cmd(userStr);
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getCommand(), "USER");
	std::vector<std::string> params = cmd.getParams();
	// Should parse as: guest, :Ronnie Reagan, tolmoon, tolsun
	ASSERT_EQ(params.size(), 4);
	EXPECT_EQ(params.at(0), "guest");
	EXPECT_EQ(params.at(1), ":Ronnie");
	EXPECT_EQ(params.at(2), "Reagan");
	EXPECT_EQ(params.at(3), "tolmoon tolsun");
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);
}

TEST(CommandClassTest, UserCommandColonInParamButNoRealnameTest) {
	std::string userStr = "USER guest tolmoon : tolsun\r\n";
	IRCCommand cmd(userStr);
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getCommand(), "USER");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 4);
	EXPECT_EQ(params.at(0), "guest");
	EXPECT_EQ(params.at(1), "tolmoon");
	EXPECT_EQ(params.at(2), ":");
	EXPECT_EQ(params.at(3), "tolsun");
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);
}

TEST(CommandClassTest, UserCommandRealnameWithSpacesTest) {
	std::string userStr = "USER guest tolmoon tolsun :Ronnie Reagan is here\r\n";
	IRCCommand cmd(userStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getCommand(), "USER");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 4);
	EXPECT_EQ(params.at(0), "guest");
	EXPECT_EQ(params.at(1), "tolmoon");
	EXPECT_EQ(params.at(2), "tolsun");
	EXPECT_EQ(params.at(3), ":Ronnie Reagan is here");
	EXPECT_EQ(cmd.getErrorNum(), "");
}

TEST(CommandClassTest, UserCommandRealnameWithColonInsideTest) {
	std::string userStr = "USER guest tolmoon tolsun :Ronnie:Reagan\r\n";
	IRCCommand cmd(userStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getCommand(), "USER");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 4);
	EXPECT_EQ(params.at(0), "guest");
	EXPECT_EQ(params.at(1), "tolmoon");
	EXPECT_EQ(params.at(2), "tolsun");
	EXPECT_EQ(params.at(3), ":Ronnie:Reagan");
	EXPECT_EQ(cmd.getErrorNum(), "");
}

// CAP LS / ACK

TEST(CommandClassTest, CapCommandTest) {
	std::string capStr = "CAP LS\r\n";
	IRCCommand cmd(capStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getCommand(), "CAP");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 1);
	EXPECT_EQ(params.at(0), "LS");
	EXPECT_EQ(cmd.getErrorNum(), "");
}

TEST(CommandClassTest, CapInvalidCommandTest) {
	std::string capStr = "CAP\r\n";
	IRCCommand cmd(capStr);
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getCommand(), "CAP");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 0);
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);
}

TEST(CommandClassTest, JoinCommandValidTest) {
	std::string joinStr = "JOIN #channel\r\n";
	IRCCommand cmd(joinStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getCommand(), "JOIN");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 1);
	EXPECT_EQ(params.at(0), "#channel");
	EXPECT_EQ(cmd.getErrorNum(), "");
}

TEST(CommandClassTest, JoinCommandMultipleChannelsTest) {
	std::string joinStr = "JOIN #channel1,#channel2,#channel3\r\n";
	IRCCommand cmd(joinStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getCommand(), "JOIN");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 1);
	EXPECT_EQ(params.at(0), "#channel1,#channel2,#channel3");
	EXPECT_EQ(cmd.getErrorNum(), "");
}



// INVITE Command Tests
TEST(CommandClassTest, InviteCommandValidTest) {
	std::string inviteStr = "INVITE Kilroy #Finnish\r\n";
	IRCCommand cmd(inviteStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getCommand(), "INVITE");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 2);
	EXPECT_EQ(params.at(0), "Kilroy");
	EXPECT_EQ(params.at(1), "#Finnish");
	EXPECT_EQ(cmd.getErrorNum(), "");
}

TEST(CommandClassTest, InviteCommandMissingChannelTest) {
	std::string inviteStr = "INVITE Kilroy\r\n";
	IRCCommand cmd(inviteStr);
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getCommand(), "INVITE");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 1);
	EXPECT_EQ(params.at(0), "Kilroy");
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);
}

TEST(CommandClassTest, InviteCommandNoParamsTest) {
	std::string inviteStr = "INVITE\r\n";
	IRCCommand cmd(inviteStr);
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getCommand(), "INVITE");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 0);
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);
}

// TOPIC Command Tests
TEST(CommandClassTest, TopicCommandViewTest) {
	std::string topicStr = "TOPIC #Finnish\r\n";
	IRCCommand cmd(topicStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getCommand(), "TOPIC");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 1);
	EXPECT_EQ(params.at(0), "#Finnish");
	EXPECT_EQ(cmd.getErrorNum(), "");
}

TEST(CommandClassTest, TopicCommandSetTest) {
	std::string topicStr = "TOPIC #Finnish :New topic here\r\n";
	IRCCommand cmd(topicStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getCommand(), "TOPIC");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 2);
	EXPECT_EQ(params.at(0), "#Finnish");
	EXPECT_EQ(params.at(1), "New topic here");
	EXPECT_EQ(cmd.getErrorNum(), "");
}

TEST(CommandClassTest, TopicCommandSetWithoutColonTest) {
	std::string topicStr = "TOPIC #Finnish NewTopic\r\n";
	IRCCommand cmd(topicStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getCommand(), "TOPIC");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 2);
	EXPECT_EQ(params.at(0), "#Finnish");
	EXPECT_EQ(params.at(1), "NewTopic");
	EXPECT_EQ(cmd.getErrorNum(), "");
}

TEST(CommandClassTest, TopicCommandNoChannelTest) {
	std::string topicStr = "TOPIC\r\n";
	IRCCommand cmd(topicStr);
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getCommand(), "TOPIC");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 0);
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);
}

TEST(CommandClassTest, TopicCommandEmptyTopicTest) {
	std::string topicStr = "TOPIC #Finnish :\r\n";
	IRCCommand cmd(topicStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getCommand(), "TOPIC");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 1);
	EXPECT_EQ(params.at(0), "#Finnish");
	EXPECT_EQ(cmd.getErrorNum(), "");
}

// KICK Command Tests
TEST(CommandClassTest, KickCommandValidTest) {
	std::string kickStr = "KICK #Finnish John\r\n";
	IRCCommand cmd(kickStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getCommand(), "KICK");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 2);
	EXPECT_EQ(params.at(0), "#Finnish");
	EXPECT_EQ(params.at(1), "John");
	EXPECT_EQ(cmd.getErrorNum(), "");
}

TEST(CommandClassTest, KickCommandWithReasonTest) {
	std::string kickStr = "KICK #Finnish John :Bad behavior\r\n";
	IRCCommand cmd(kickStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getCommand(), "KICK");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 3);
	EXPECT_EQ(params.at(0), "#Finnish");
	EXPECT_EQ(params.at(1), "John");
	EXPECT_EQ(params.at(2), "Bad behavior");
	EXPECT_EQ(cmd.getErrorNum(), "");
}

TEST(CommandClassTest, KickCommandMissingUserTest) {
	std::string kickStr = "KICK #Finnish\r\n";
	IRCCommand cmd(kickStr);
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getCommand(), "KICK");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 1);
	EXPECT_EQ(params.at(0), "#Finnish");
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);
}

TEST(CommandClassTest, KickCommandNoParamsTest) {
	std::string kickStr = "KICK\r\n";
	IRCCommand cmd(kickStr);
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getCommand(), "KICK");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 0);
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);
}

TEST(CommandClassTest, KickCommandWithLongReasonTest) {
	std::string kickStr = "KICK #Finnish John :This is a very long reason for kicking\r\n";
	IRCCommand cmd(kickStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getCommand(), "KICK");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 3);
	EXPECT_EQ(params.at(0), "#Finnish");
	EXPECT_EQ(params.at(1), "John");
	EXPECT_EQ(params.at(2), "This is a very long reason for kicking");
	EXPECT_EQ(cmd.getErrorNum(), "");
}

// PRIVMSG Command Tests
TEST(CommandClassTest, PrivmsgCommandToChannelTest) {
	std::string privmsgStr = "PRIVMSG #channel :Hello world\r\n";
	IRCCommand cmd(privmsgStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getCommand(), "PRIVMSG");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 2);
	EXPECT_EQ(params.at(0), "#channel");
	EXPECT_EQ(params.at(1), "Hello world");
	EXPECT_EQ(cmd.getErrorNum(), "");
}

TEST(CommandClassTest, PrivmsgCommandToUserTest) {
	std::string privmsgStr = "PRIVMSG John :Hi there!\r\n";
	IRCCommand cmd(privmsgStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getCommand(), "PRIVMSG");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 2);
	EXPECT_EQ(params.at(0), "John");
	EXPECT_EQ(params.at(1), "Hi there!");
	EXPECT_EQ(cmd.getErrorNum(), "");
}

TEST(CommandClassTest, PrivmsgCommandWithoutColonTest) {
	std::string privmsgStr = "PRIVMSG #channel Hello\r\n";
	IRCCommand cmd(privmsgStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getCommand(), "PRIVMSG");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 2);
	EXPECT_EQ(params.at(0), "#channel");
	EXPECT_EQ(params.at(1), "Hello");
	EXPECT_EQ(cmd.getErrorNum(), "");
}

TEST(CommandClassTest, PrivmsgCommandNoMessageTest) {
	std::string privmsgStr = "PRIVMSG #channel\r\n";
	IRCCommand cmd(privmsgStr);
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getCommand(), "PRIVMSG");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 2);
	EXPECT_EQ(params.at(0), "#channel");
	EXPECT_EQ(params.at(1), "");
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);
}

TEST(CommandClassTest, PrivmsgCommandEmptyMessageTest) {
	std::string privmsgStr = "PRIVMSG #channel :\r\n";
	IRCCommand cmd(privmsgStr);
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getCommand(), "PRIVMSG");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 2);
	EXPECT_EQ(params.at(0), "#channel");
	EXPECT_EQ(params.at(1), "");
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);
}

TEST(CommandClassTest, PrivmsgCommandNoTargetTest) {
	std::string privmsgStr = "PRIVMSG\r\n";
	IRCCommand cmd(privmsgStr);
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getCommand(), "PRIVMSG");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 0);
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);
}

TEST(CommandClassTest, PrivmsgCommandLongMessageTest) {
	std::string privmsgStr = "PRIVMSG #channel :This is a very long message with multiple words\r\n";
	IRCCommand cmd(privmsgStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getCommand(), "PRIVMSG");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 2);
	EXPECT_EQ(params.at(0), "#channel");
	EXPECT_EQ(params.at(1), "This is a very long message with multiple words");
	EXPECT_EQ(cmd.getErrorNum(), "");
}

TEST(CommandClassTest, PrivmsgCommandWithSpecialCharsTest) {
	std::string privmsgStr = "PRIVMSG #channel :Hello! How are you? :)\r\n";
	IRCCommand cmd(privmsgStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getCommand(), "PRIVMSG");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 2);
	EXPECT_EQ(params.at(0), "#channel");
	EXPECT_EQ(params.at(1), "Hello! How are you? :)");
	EXPECT_EQ(cmd.getErrorNum(), "");
}



// PING ft_irc.42.de
TEST(CommandClassTest, PingCommandValidTest) {
	std::string pingStr = "PING ft_irc.42.de\r\n";
	IRCCommand cmd(pingStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getCommand(), "PING");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 1);
	EXPECT_EQ(params.at(0), "ft_irc.42.de");
	EXPECT_EQ(cmd.getErrorNum(), "");
}

TEST(CommandClassTest, PingCommandNoParamsTest) {
	std::string pingStr = "PING\r\n";
	IRCCommand cmd(pingStr);
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getCommand(), "PING");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 0);
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);
}

TEST(CommandClassTest, PingCommandEmptyParamTest) {
	std::string pingStr = "PING :\r\n";
	IRCCommand cmd(pingStr);
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getCommand(), "PING");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 1);
	EXPECT_EQ(params.at(0), ":");
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);
}

TEST(CommandClassTest, PingCommandWithExtraParamsTest) {
	std::string pingStr = "PING ft_irc.42.de extra\r\n";
	IRCCommand cmd(pingStr);
	EXPECT_EQ(cmd.isValid(), false);
	EXPECT_EQ(cmd.getCommand(), "PING");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 2);
	EXPECT_EQ(params.at(0), "ft_irc.42.de");
	EXPECT_EQ(params.at(1), "extra");
	EXPECT_EQ(cmd.getErrorNum(), ERR_NEEDMOREPARAMS);
}

// PONG ft_irc.42.de

// PONG Command Tests
TEST(CommandClassTest, PongCommandValidTest) {
	std::string pongStr = "PONG ft_irc.42.de\r\n";
	IRCCommand cmd(pongStr);
	EXPECT_EQ(cmd.isValid(), true);
	EXPECT_EQ(cmd.getCommand(), "PONG");
	std::vector<std::string> params = cmd.getParams();
	ASSERT_EQ(params.size(), 1);
	EXPECT_EQ(params.at(0), "ft_irc.42.de");
	EXPECT_EQ(cmd.getErrorNum(), "");
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


// command like string returns information by command
// if error occurs, command can return error message by accepting client
// if command is valid, client

// Create ChannelsManager(clients vector, clientMSG))
