#ifndef NICK_HPP
#define NICK_HPP

// Forward declarations
class Server;
class Client;
struct Command;

// NICK command handler
// Sets or changes client nickname
void handleNick(Server& server, Client& client, const Command& cmd);

#endif // NICK_HPP

