#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client {
public:
    Client(int fd);
    ~Client();
    
    const std::string& getNickname() const;
    const std::string& getUsername() const;
    const std::string& getHostname() const;
    std::string getPrefix() const;
    
private:
    int fd_;
};

#endif