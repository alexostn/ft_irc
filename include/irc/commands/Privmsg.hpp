#ifndef PRIVMSG_HPP
#define PRIVMSG_HPP

// Forward declarations
class Server;
class Client;
struct Command;

// PRIVMSG command handler
// Sends a message to a channel or user
void handlePrivmsg(Server& server, Client& client, const Command& cmd);

#endif // PRIVMSG_HPP

