#ifndef QUIT_HPP
#define QUIT_HPP

// Forward declarations
class Server;
class Client;
struct Command;

// QUIT command handler
// Disconnects client from server
void handleQuit(Server& server, Client& client, const Command& cmd);

#endif // QUIT_HPP

