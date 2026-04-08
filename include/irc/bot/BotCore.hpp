#ifndef BOT_CORE_HPP
#define BOT_CORE_HPP

#include <string>
#include <vector>

class BotCore
{
public:
    BotCore(const std::string& host, int port, const std::string& pass,
            const std::string& nick, const std::string& channel);
    ~BotCore();

    bool connect();
    bool registerIRC();
    std::vector<std::string> tick(int timeout_ms);
    bool reconnect(int max_attempts);
    bool isConnected() const;
    void sendRaw(const std::string& msg);
    void close();
    void stop();
    bool isRunning() const;

private:
    std::string host_;
    int port_;
    std::string pass_;
    std::string nick_;
    std::string channel_;
    int fd_;
    std::string recv_buffer_;
    bool running_;

    bool connectToServer();
    std::vector<std::string> extractLines();
    void handlePing(const std::string& line);
};

#endif
