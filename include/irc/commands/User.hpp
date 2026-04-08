#ifndef USER_HPP
#define USER_HPP

// Forward declarations
class Server;
class Client;
struct Command;

// USER command handler
// Sets username and realname for client
void handleUser(Server& server, Client& client, const Command& cmd);

#endif // USER_HPP

