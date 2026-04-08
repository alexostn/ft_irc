#ifndef PING_HPP
#define PING_HPP

// Forward declarations
class Server;
class Client;
struct Command;

// PING command handler
// Server sends PING to check if client is alive
void handlePing(Server& server, Client& client, const Command& cmd);

#endif // PING_HPP

