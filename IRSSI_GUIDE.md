# Irssi IRC Client Guide for ft_irc

## Connecting to Your Server

```bash
irssi
```

Once Irssi is running, connect to your server:
```
/connect localhost 1201 123
```
- `localhost` - server address
- `1201` - port
- `123` - password

## Basic Commands

### Navigation
- **Alt + number** - Switch between windows (Alt+1, Alt+2, etc.)
- **Alt + ←/→** - Navigate between windows
- **/window close** - Close current window

### Channel Commands
```
/join #channel          - Join a channel
/part #channel          - Leave a channel
/topic #channel New     - Set channel topic (if operator)
/names #channel         - List users in channel
```

### Messaging
```
/msg nickname message   - Send private message
Hello everyone!         - Send message to current channel/window
```

### User Management (if operator)
```
/kick #channel nickname reason     - Kick user from channel
/invite nickname #channel          - Invite user to channel
/mode #channel +o nickname         - Give operator status
/mode #channel +i                  - Set channel invite-only
/mode #channel +t                  - Set channel topic protection
/mode #channel +k password         - Set channel password
/mode #channel +l 10               - Set user limit to 10
```

### Information
```
/whois nickname         - Get user information
/list                   - List channels (not implemented)
/who #channel           - List users in channel (not implemented)
```

### Connection
```
/nick newnick           - Change your nickname
/quit                   - Disconnect and exit
/disconnect             - Disconnect from server
```

## Tips

1. **Input is at the bottom** - Type your commands/messages in the bottom line
2. **Active window** - Your messages go to the currently active window
3. **Status window** - Window 1 is always the status window
4. **Join a channel first** - Before chatting, use `/join #test`
5. **Multiple connections** - You can connect to multiple servers

## Common Workflow

```bash
# Start server (in one terminal)
./ft_irc_serv 1201 123

# Start Irssi (in another terminal)
irssi

# In Irssi:
/connect localhost 1201 123
/join #test
Hello everyone!
/quit
```

## Troubleshooting

- **Can't type?** - Make sure you're in the input area (bottom of screen)
- **Invalid command?** - Check that the command is supported by ft_irc
- **Connection issues?** - Verify server is running on correct port
- **Strange errors?** - Some Irssi features may not be implemented in ft_irc

## Testing Multi-User Chat

Open multiple terminals:

**Terminal 1:**
```bash
./ft_irc_serv 1201 123
```

**Terminal 2:**
```bash
irssi
/connect localhost 1201 123
/nick alice
/join #test
```

**Terminal 3:**
```bash
irssi
/connect localhost 1201 123
/nick bob
/join #test
```

Now Alice and Bob can chat in #test!
