#ifndef INVITE_HPP
#define INVITE_HPP

// Forward declarations
class Server;
class Client;
struct Command;

// INVITE command handler
// Invites a user to an invite-only channel
void handleInvite(Server& server, Client& client, const Command& cmd);

#endif // INVITE_HPP

