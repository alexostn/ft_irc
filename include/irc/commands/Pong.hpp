#ifndef PONG_HPP
#define PONG_HPP

// Forward declarations
class Server;
class Client;
struct Command;

// PONG command handler
// Client responds to server's PING with PONG
void handlePong(Server& server, Client& client, const Command& cmd);

#endif // PONG_HPP

