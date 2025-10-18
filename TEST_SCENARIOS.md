# FT_IRC — Test Scenarios

This document lists focused manual and automated test scenarios to exercise the ft_irc server (happy paths, edge-cases and likely crash points). Each scenario includes short steps and expected results and points to code locations to exercise.

> Quick note: run unit tests via the `googletests` folder or integration tests by starting `./ft_irc_serv <port> <password>` then connecting with a telnet / netcat / custom client.

---

## 1. Basic registration (happy path) ✅
- Steps:
  1. Connect to server (tcp).
  2. Send `PASS <password>`.
  3. Send `NICK alice`.
  4. Send `USER alice 0 * :Real Name`.
- Expected:
  - Registration completes and server sends welcome message.
- Code to exercise: `ChannelsClientsManager::registerClient`, `Reply::welcome`, `IRCCommand::handlePassCmd`.

## 2. Wrong password rejection
- Steps:
  1. Connect.
  2. Send `PASS wrong`.
  3. Try `NICK/USER` sequence.
- Expected:
  - Server rejects registration with a password error reply; client not registered.
- Code: `ChannelsClientsManager::registerClient`, `Reply::passwordMismatch`.

## 4. Duplicate nickname handling ✅
- Steps:
  1. Client A registers as `bob`.
  2. Client B attempts `NICK bob` and registers.
- Expected:
  - Client B receives `433 nickname in use` and cannot use `bob`.
- Code: `ChannelsClientsManager::isNickInUse`, `Reply::nicknameInUse`.

## 5. JOIN creates channel + operator assignment ✅
- Steps:
  1. Registered client sends `JOIN #chat`.
- Expected:
  - Channel `#chat` is created and the user is marked operator; server sends JOIN/TOPIC replies.
- Code: `ChannelsClientsManager::executeJoin`, `Channel::addOperator`.

## 6. JOIN multiple channels in one command ✅
- Steps:
  1. `JOIN #a,#b,#c`.
- Expected:
  - Client joins all listed channels; multiple join replies.
- Code: `ChannelsClientsManager::executeJoin`.

## 7. JOIN with key (+k) and wrong key ✅
- Steps:
  1. Create channel with `MODE #chan +k secret`.
  2. Another client sends `JOIN #chan wrongkey`.
- Expected:
  - Server replies with `475` (wrong key) for wrong key; correct key allows join.
- Code: `ChannelsClientsManager::handleModeFlags`, `executeJoin`.

## 8. JOIN invite-only (+i) enforcement
- Steps:
  1. Set channel `#only` to `+i`.
  2. Non-invited client tries `JOIN #only`.
- Expected:
  - Server replies `473` (invite-only) and blocks join.
- Code: `executeJoin`, `handleModeFlags`.

## 9. Channel full (limit +l)
- Steps:
  1. Set `+l N` small limit.
  2. Have >N clients attempt to join.
- Expected:
  - Server replies `471` when channel full.
- Code: `Channel` limit handling, `executeJoin`.

## 10. PRIVMSG to channel (broadcast)
- Steps:
  1. user1 and user2 join `#chat`.
  2. user1 sends `PRIVMSG #chat :hello`.
- Expected:
  - user2 receives formatted PRIVMSG from user1.
- Code: `ChannelsClientsManager::executePrivmsg`, `Channel::broadcast`.

## 11. PRIVMSG to user (private message)
- Steps:
  1. `PRIVMSG targetnick :hi`.
- Expected:
  - targetnick receives message; if nick does not exist, server replies `401`.
- Code: `executePrivmsg`, `getClientByNickname`.

## 12. INVITE flow + invite-only join
- Steps:
  1. Operator sends `INVITE user #chan`.
  2. Invited user sends `JOIN #chan`.
- Expected:
  - Invited user can join an invite-only channel after invite.
- Code: `executeInvite`, `executeJoin`.

## 13. MODE tests: +o / -o and permission checks
- Steps:
  1. Operator does `MODE #chan +o othernick`.
  2. Non-operator tries privileged `MODE` changes.
- Expected:
  - Operator can grant/revoke ops; non-operator receives `482` if not allowed.
- Code: `handleModeFlags`.

## 14. TOPIC set/view with +t protection
- Steps:
  1. Set `+t` on channel.
  2. Non-operator tries `TOPIC #chan :text`.
  3. Operator sets topic.
- Expected:
  - Non-operator gets `482`; operator sets topic and topic is broadcast.
- Code: `executeTopic`.

## 15. KICK and rejoin losing operator status
- Steps:
  1. Operator kicks user.
  2. Kicked user rejoins.
- Expected:
  - Kicked user removed, receives message; on rejoin they are not operator unless reassigned.
- Code: `executeKick`, `Channel::removeOperator`.

## 16. PING/PONG keepalive & timeout
- Steps:
  1. Server sends `PING` and expects `PONG`.
  2. Client does not respond within timeout.
- Expected:
  - PONG accepted if correct; if silent too long, server disconnects client.
- Code: `ChannelsClientsManager::sendPingToClient`, server timers in `Server.cpp`.

## 17. Malformed commands / missing params
- Steps:
  1. Send `KICK #chan` (missing nick) or `MODE` without params.
- Expected:
  - Server replies `461`/`ERR_NEEDMOREPARAMS`.
- Code: `IRCCommand` handlers, `Reply::needMoreParams`.

## 18. Concurrent messaging and race conditions
- Steps:
  1. Start many clients sending `PRIVMSG` to same channel quickly.
- Expected:
  - Server handles without crash; messages delivered.
- Code: `Channel::broadcast`, `executePrivmsg`.

## 19. Client abrupt disconnect while in channels
- Steps:
  1. Client closes TCP connection unexpectedly.
- Expected:
  - Server removes client cleanly, removes them from channels, deletes empty channels.
- Code: `ChannelsClientsManager::removeClient`, `Server::removeClient`.

## 20. Large/edge payloads and buffer boundaries
- Steps:
  1. Send oversized PRIVMSG near buffer limit.
  2. Send many commands in one TCP packet (multiple `\r\n`).
- Expected:
  - Server correctly parses multiple messages; no buffer overflow; boundaries handled.
- Code: `Client::hasCompleteMessage`, `IRCCommand::processCommand`.

---

## How to run quick manual tests
- Build with `make re`.
- Start server:
```
./ft_irc_serv 1201 mypassword
```
- Connect with telnet or netcat:
```
nc 127.0.0.1 1201
```
- Send commands (each terminated with `\r\n`).

## Tips for debugging crashes
- Build with AddressSanitizer:
```
make re CXXFLAGS="-g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -std=c++98 -Wall -Wextra"
```
- Run in gdb if segfaults appear:
```
gdb --args ./ft_irc_serv 1201 mypassword
run
bt full
```

---

If you want, I can also:
- Add these as automated googletest tests under `googletests/tests/`.
- Create small Python scripts to exercise scenarios automatically (connect, send commands, assert replies).

