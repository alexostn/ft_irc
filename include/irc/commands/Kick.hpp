#ifndef KICK_HPP
#define KICK_HPP

// Forward declarations
class Server;
class Client;
struct Command;

// KICK command handler
// Removes a user from a channel (operator command)
void handleKick(Server& server, Client& client, const Command& cmd);

#endif // KICK_HPP

