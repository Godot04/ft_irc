# Client Capability Negotiation - Not Implemented
Connection Registration
Upon connecting to the IRC server, clients need a way to negotiate capabilities (protocol extensions) with the server. Negotiation is done with the CAP command, using the subcommands below to list and request capabilities.

Upon connecting to the server, the client attempts to start the capability negotiation process, as well as sending the standard NICK/USER commands (to complete registration if the server doesn’t support capability negotiation).

Upon connecting to the IRC server, clients SHOULD send one of the following messages:

CAP LS [version] to discover the available capabilities on the server.
CAP REQ to blindly request a particular set of capabilities.
Following this, the client MUST send the standard NICK and USER IRC commands.

Upon receiving either a CAP LS or CAP REQ command during connection registration, the server `MUST not` complete registration until the client sends a CAP END command to indicate that capability negotiation has ended. This allows clients to request their desired capabilities before completing registration.

**Once capability negotiation has completed with a client-sent `CAP END` command, registration continues as normal.**

With the above registration process, clients will either receive a CAP message indicating that registration is paused while capability negotiation happens, or registration will complete immediately, indicating that the server doesn’t support capability negotiation.

the response to CAP LS or CAP REQ is: CAP * LS :
Source: https://ircv3.net/specs/extensions/capability-negotiation.html