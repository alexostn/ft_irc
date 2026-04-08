#ifndef TOPIC_HPP
#define TOPIC_HPP

// Forward declarations
class Server;
class Client;
struct Command;

// TOPIC command handler
// Sets or gets channel topic
void handleTopic(Server& server, Client& client, const Command& cmd);

#endif // TOPIC_HPP

