#ifndef BOT_COMMANDS_HPP
#define BOT_COMMANDS_HPP

#include <string>
#include <ctime>

class BotCore;

class BotCommands
{
public:
    BotCommands(BotCore* core, const std::string& channel, const std::string& nick);
    ~BotCommands();

    void dispatch(const std::string& line);
    void sendAlert();

private:
    BotCore* core_;
    std::string channel_;
    std::string nick_;
    time_t start_time_;

    bool parsePrivmsg(const std::string& line,
                      std::string& sender,
                      std::string& target,
                      std::string& text);
    void handle(const std::string& sender,
                const std::string& target,
                const std::string& text);

    void cmdPing(const std::string& reply);
    void cmdEcho(const std::string& reply, const std::string& text);
    void cmdTime(const std::string& reply);
    void cmdUptime(const std::string& reply);
    void cmdHelp(const std::string& reply);
    std::string formatUptime() const;
};

#endif
