#ifndef JOIN_HPP
#define JOIN_HPP

// Forward declarations
class Server;
class Client;
struct Command;

// JOIN command handler
// Client joins a channel
void handleJoin(Server& server, Client& client, const Command& cmd);

#endif // JOIN_HPP

