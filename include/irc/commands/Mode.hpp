#ifndef MODE_HPP
#define MODE_HPP

// Forward declarations
class Server;
class Client;
struct Command;

// MODE command handler
// Sets or gets channel or user modes
void handleMode(Server& server, Client& client, const Command& cmd);

#endif // MODE_HPP

