#ifndef POLLER_HPP
#define POLLER_HPP

#include <vector>
#include <poll.h>

class Server;  // Forward declaration

// Poller class - ONLY class that calls poll() system call
// Manages file descriptor polling and event detection
// See TEAM_CONVENTIONS.md: Only Poller class calls poll()
class Poller {
public:
    // Constructor
    Poller(Server* server);
    
    // Destructor
    ~Poller();
    
    // Add file descriptor to watch list
    void addFd(int fd, short events);
    
    // Remove file descriptor from watch list
    void removeFd(int fd);
    
    // Set/change events to watch for a file descriptor
    void setEvents(int fd, short events);
    
    // Main polling loop - ONLY place poll() is called
    // Returns number of ready file descriptors
    int poll(int timeout = -1);
    
    // Process events after poll() returns
    void processEvents();
    
    // Check if a file descriptor has specific event
    bool hasEvent(int fd, short event) const;
    
private:
    Server* server_;
    std::vector<struct pollfd> pollfds_;  // List of file descriptors to poll
    
    // Helper methods
    int findFdIndex(int fd) const;
    void updatePollfds();
};

#endif // POLLER_HPP

