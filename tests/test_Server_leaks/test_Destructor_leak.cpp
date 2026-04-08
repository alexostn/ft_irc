#include "irc/Server.hpp"
#include "irc/Config.hpp"
#include <iostream>

// Simple Server destructor test — just construction + destruction
// The fix for memory leaks should be in Server.cpp destructor:
// - delete all buffers_ map entries  
// - delete all channels_ map entries
int main() {
    std::cout << "Testing Server destructor memory cleanup..." << std::endl;
    
    {
        Config cfg(6667, "pass");
        Server server(cfg);
        
        // Server destructor will be called here when server goes out of scope
        // If the destructor properly deletes buffers_ and channels_ maps,
        // valgrind should show no memory leaks
    }
    
    std::cout << "Server destructor test completed. Run with valgrind to check for leaks." << std::endl;
    return 0;
}