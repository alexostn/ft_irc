// Main entry point for IRC server
// Parse command line arguments, create Config, start Server

#include "irc/Config.hpp"
#include "irc/Server.hpp"
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return 1;
    }
    
    try {
        Config config = Config::parseArgs(argc, argv);
        Server server(config);
        server.start();
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
