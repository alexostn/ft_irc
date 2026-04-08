// Poller implementation
// ONLY class that calls poll() system call - see TEAM_CONVENTIONS.md
// Manages file descriptor polling and event notification

#include "irc/Poller.hpp"
#include "irc/Server.hpp"
#include <algorithm>
#include <poll.h>
#include <iostream>
#include <cerrno>
#include <cstring>

// DONE: Implement Poller::Poller(Server* server)
Poller::Poller(Server* server) : server_(server) {
    pollfds_.reserve(64); // Reserve space for 64 file descriptors
    std::cout << "[Poller] Initialized" << std::endl;
}

// DONE: Implement Poller::~Poller()
Poller::~Poller() {
    std::cout << "[Poller] Destroyed" << std::endl;
}

// DONE: Implement Poller::addFd(int fd, short events)
void Poller::addFd(int fd, short events) {
    // Check if fd already exists
    if (findFdIndex(fd) != -1) {
        std::cerr << "[Poller] WARNING: fd=" << fd 
                    << " already exists!" << std::endl;
        return;
    }
    
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = events;
    pfd.revents = 0;
    pollfds_.push_back(pfd);

    std::cout << "[Poller] Added fd=" << fd << std::endl;
}

// DONE: Implement Poller::removeFd(int fd)
void Poller::removeFd(int fd) {
    int index = findFdIndex(fd);
    if (index != -1) {
        pollfds_.erase(pollfds_.begin() + index);
        std::cout << "[Poller] Removed fd=" << fd << std::endl;
    } else {
        std::cerr << "[Poller] WARNING: fd=" << fd << " not found!" << std::endl;
    }
}

// DONE: Implement Poller::setEvents(int fd, short events)
// Change the events to watch for a specific file descriptor (e.g., add POLLOUT)
void Poller::setEvents(int fd, short events) {
    int index = findFdIndex(fd);
    if (index != -1) {
        pollfds_[index].events = events;
        std::cout << "[Poller] setEvents fd=" << fd << " events=" << events << std::endl;
    } else {
        std::cerr << "[Poller] WARNING: setEvents failed - fd=" << fd << " not found!" << std::endl;
    }
}

// DONE: Implement Poller::poll(int timeout)
// ONLY PLACE poll() IS CALLED - see TEAM_CONVENTIONS.md
int Poller::poll(int timeout) {
    if (pollfds_.empty()) {
        return 0;
    }
    
    int ready = ::poll(&pollfds_[0], pollfds_.size(), timeout);  // Call global poll()
    if (ready < 0) {
        if (errno == EINTR) {
            return 0;  // Signal interrupt - normal
        }
        std::cerr << "[Poller] poll() error: " << strerror(errno) << std::endl;
        return 0;
    }
    return ready;
}

// DONE: Implement Poller::processEvents()
void Poller::processEvents() {
    int serverFd = server_->getServerFd();

    for (size_t i = 0; i < pollfds_.size(); ++i) {
        int fd = pollfds_[i].fd;
        short revents = pollfds_[i].revents;
        
        if (revents == 0) continue;
        
        std::cout << "[Poller] fd=" << fd << " revents=0x" 
                    << std::hex << revents << std::dec;
        
        // Log events
        if (revents & POLLIN)  std::cout << " POLLIN";
        if (revents & POLLHUP) std::cout << " POLLHUP";
        if (revents & POLLERR) std::cout << " POLLERR";
        std::cout << std::endl;
        
        if (fd == serverFd) {
            // New connection on server socket
            if (revents & POLLIN) {
                server_->handleNewConnection();
            }
        } else {
            // Client socket: handle POLLOUT (drain send queue) FIRST
            if (revents & POLLOUT) {
                server_->flushClientSendQueue(fd);
            }
            // Then handle POLLIN or POLLHUP
            if (revents & (POLLIN | POLLHUP)) {
                // handleClientInput handles recv() == 0
                server_->handleClientInput(fd);
            }
            // POLLERR: critical socket error
            else if (revents & POLLERR) {
                std::cerr << "[Poller] POLLERR on fd=" << fd << std::endl;
                server_->disconnectClient(fd);
            }
        }
    }
}

// Helper methods
// - findFdIndex(): internal search (used by addFd, removeFd, hasEvent)
// - hasEvent(): check specific event for fd (useful for Server POLLOUT handling)
// - updatePollfds(): ensures vector consistency (future: rebuild from map for transcendence)

// DONE: Implement Poller::hasEvent(int fd, short event) const
bool Poller::hasEvent(int fd, short event) const {
    int index = findFdIndex(fd);
    if (index == -1) return false;
    return (pollfds_[index].revents & event) != 0;
}

// DONE: Implement Poller::findFdIndex(int fd) const
int Poller::findFdIndex(int fd) const {
    for (size_t i = 0; i < pollfds_.size(); ++i) {
        if (pollfds_[i].fd == fd) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// DONE: Implement Poller::updatePollfds()
// Current: ensures vector consistency
// Future: will rebuild from map<fd, FdInfo> for transcendence (roles, priorities)
void Poller::updatePollfds() {
    // Current implementation: clear revents for consistency
    for (size_t i = 0; i < pollfds_.size(); ++i) {
        pollfds_[i].revents = 0;
    }
    
    // Future (transcendence): rebuild from map with priorities
    // When adding std::map<int, FdInfo> descriptors_:
    /*
    pollfds_.clear();
    for (auto& kv : descriptors_) {
        if (kv.second.active) {
            struct pollfd pfd = {kv.first, kv.second.events, 0};
            pollfds_.push_back(pfd);
        }
    }
    */
}
