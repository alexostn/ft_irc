#ifndef PASS_HPP
#define PASS_HPP

// Forward declarations
class Server;
class Client;
struct Command;

// PASS command handler
// Sets password for client connection
void handlePass(Server& server, Client& client, const Command& cmd);

#endif // PASS_HPP

