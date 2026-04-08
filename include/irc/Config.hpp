#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>

// Configuration class for IRC server settings
// Manages port, password, and other server configuration
class Config {
public:
    // Constructor: initialize with default or provided values
    Config(int port, const std::string& password);
    
    // Getters
    int getPort() const;
    const std::string& getPassword() const;
    static const std::string& getServerName();
    
    // Setters (if needed)
    void setPort(int port);
    void setPassword(const std::string& password);
    
    // Parse configuration from command line arguments
    static Config parseArgs(int argc, char** argv);
    
private:
    int port_;
    std::string password_;
    static const std::string serverName_;
    // Add other configuration options as needed
};

#endif // CONFIG_HPP
