# ft_irc - IRC Server

![42 school](https://img.shields.io/badge/42-School-000000?style=flat-square&logo=42&logoColor=white)
![C++](https://img.shields.io/badge/C%2B%2B-00599C?style=flat-square&logo=c%2B%2B&logoColor=white)
![Poll](https://img.shields.io/badge/poll-based%20server-success?style=flat-square)

## About

**ft_irc** is a project that implements a lightweight IRC server in C++98. The server accepts multiple clients at the same time, keeps them registered, and routes IRC messages through a non-blocking event loop.

## Project Goals

- Build a server that listens on a configurable port
- Protect the server with a connection password
- Support several clients without blocking the event loop
- Parse IRC commands and return protocol-compliant replies
- Manage users, channels, and server-side state cleanly

## IRC Server Model

The server follows a typical IRC workflow:

- A client connects to the server through TCP
- The client authenticates with `PASS`
- The client registers a nickname with `NICK`
- The client registers user information with `USER`
- After registration, the client can interact with channels and other users

The implementation is centered around a poll-driven loop that watches the server socket and all connected client sockets. New connections are accepted in non-blocking mode, and each connected client is tracked by file descriptor.

## Rules and Constraints

### Program Arguments

```bash
./ircserv <port> <password>
```

- **port**: TCP port used by the IRC server
- **password**: connection password required by clients during registration

### Server Behavior

- The server must reject invalid command-line arguments
- The listening socket is configured in non-blocking mode
- Client sockets are also non-blocking
- The event loop must keep running until the server is stopped
- Input is parsed line by line from each connected client
- Replies are sent using IRC-style numeric codes and messages

## Key Features

### 1. Connection Handling

Clients connect over TCP and are added to the server’s polling set. Each client is wrapped in a server-side object so the connection state, hostname, nickname, and registration status can be tracked independently.

### 2. Registration Flow

The server supports the standard IRC registration steps with password checking and identity setup. The codebase already contains support for commands such as `PASS`, `NICK`, `USER`, and `CAP`, which are handled during the registration phase.

### 3. Channel and User Management

Connected clients can be organized into channels, and the server keeps shared structures for looking up clients, managing membership, and delivering command results.

### 4. Reply Formatting

Protocol responses are centralized in helper code so replies stay consistent across commands. This keeps numeric replies, welcome messages, and error messages aligned with IRC formatting.

### 5. Graceful Shutdown

The server handles termination cleanly, closes sockets, and releases allocated client objects when it stops.

## Compilation

### Build the Program

```bash
make
```

This creates the `ircserv` executable.

### Available Make Commands

- `make` - Build the server
- `make clean` - Remove object files
- `make fclean` - Remove object files and the executable
- `make re` - Rebuild everything from scratch

### Compiler Flags

- `-Wall -Wextra -Werror` - Enable strict warnings
- `-std=c++98` - Use the required C++ standard

## Usage

### Basic Example

```bash
./ircserv 6667 mypassword
```

### Expected Workflow

1. Start the server on a chosen port
2. Connect with an IRC client or a socket tester
3. Send `PASS`, `NICK`, and `USER`
4. Join channels and exchange messages once registered

### Testing Ideas

- Connect with a valid password and verify registration replies
- Connect with an invalid password and check rejection behavior
- Open multiple clients and confirm the server keeps them responsive
- Send partial lines and verify buffered parsing behaves correctly
- Stop the server with `Ctrl+C` and verify sockets close cleanly

## Implementation Structure

The project is split into small modules:

- **main.cpp** - Argument parsing and server startup
- **Server.cpp** - Socket setup, polling loop, connection handling
- **Client.cpp** - Per-client state and message buffering
- **Channel.cpp** - Channel state and membership logic
- **IRCCommand.cpp** - Command parsing and representation
- **ChannelsClientsManager.cpp** - Shared client and channel operations
- **Reply.cpp** - IRC replies and error helpers

## Technical Details

- **Language**: C++98
- **Networking**: TCP sockets
- **I/O Model**: non-blocking sockets with `poll()`
- **Server Style**: single-process, event-driven
- **Protocol**: IRC-inspired message handling

## Authors

**opopov** and **zkhojazo**
