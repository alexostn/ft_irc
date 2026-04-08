#ifndef PART_HPP
#define PART_HPP

// Forward declarations
class Server;
class Client;
struct Command;

// PART command handler
// Client leaves a channel
void handlePart(Server& server, Client& client, const Command& cmd);

#endif // PART_HPP

