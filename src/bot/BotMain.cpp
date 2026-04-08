// BotMain.cpp — Main event loop: connect, poll, dispatch, alert

#include "irc/bot/BotCore.hpp"
#include "irc/bot/BotCommands.hpp"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <csignal>

#define ALERT_INTERVAL 60  // Send status alert every 60 seconds

// Global pointer for signal handler — C++98 has no lambdas, global is the only way
static BotCore* g_bot = NULL;

// Unified signal handler — same pattern as Server.cpp (sigaction-based)
static void signal_handler(int sig)
{
    (void)sig;
    if (g_bot)
        g_bot->stop();  // Sets running_ = false → exits while loop cleanly
}

int main(int argc, char* argv[])
{
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0]
                  << " <host> <port> <pass> <nick> <channel>" << std::endl;
        std::cerr << "Example: " << argv[0]
                  << " 127.0.0.1 6667 testpass playbot #general" << std::endl;
        return 1;
    }

    std::string host    = argv[1];
    int         port    = std::atoi(argv[2]);
    std::string pass    = argv[3];
    std::string nick    = argv[4];
    std::string channel = argv[5];

    BotCore     core(host, port, pass, nick, channel);
    BotCommands cmds(&core, channel, nick);

    g_bot = &core;

    // Setup SIGINT, SIGTERM, SIGQUIT — graceful shutdown (unified with Server.cpp)
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;       // Auto-restart poll() after signal — no EINTR handling needed
    sigaction(SIGINT,  &sa, NULL);  // Ctrl+C
    sigaction(SIGTERM, &sa, NULL);  // kill <pid> / timeout
    sigaction(SIGQUIT, &sa, NULL);  // Ctrl+\ — prevent core dump

    // Setup SIGPIPE — ignore broken pipe on send() to disconnected server
    struct sigaction sa_pipe;
    sa_pipe.sa_handler = SIG_IGN;
    sigemptyset(&sa_pipe.sa_mask);
    sa_pipe.sa_flags = 0;
    sigaction(SIGPIPE, &sa_pipe, NULL);

    if (!core.connect()) {
        std::cerr << "[ERR] Failed to connect" << std::endl;
        g_bot = NULL;
        return 1;
    }

    if (!core.registerIRC()) {
        std::cerr << "[ERR] Failed to register" << std::endl;
        g_bot = NULL;
        return 1;
    }

    std::cout << "[BOT] Entering main loop (ALERT_INTERVAL=" << ALERT_INTERVAL
              << "s)" << std::endl;

    time_t last_alert = time(NULL);

    while (core.isRunning()) {  // isRunning() = false when signal_handler calls stop()
        time_t now = time(NULL);
        long elapsed = static_cast<long>(difftime(now, last_alert));
        int timeout_ms = ((ALERT_INTERVAL - elapsed) > 0)
                       ? (ALERT_INTERVAL - elapsed) * 1000
                       : 0;

        std::vector<std::string> lines = core.tick(timeout_ms);

        for (size_t i = 0; i < lines.size(); ++i) {
            std::cout << "[BOT<<] " << lines[i] << std::endl;
            cmds.dispatch(lines[i]);
        }

        // Send alert if poll() timeout expired
        now = time(NULL);
        elapsed = static_cast<long>(difftime(now, last_alert));
        if (elapsed >= ALERT_INTERVAL) {
            cmds.sendAlert();
            last_alert = now;
        }

        // Reconnect if lost connection
        if (!core.isConnected() && core.isRunning()) {
            std::cout << "[BOT] Connection lost, attempting reconnect..." << std::endl;
            if (!core.reconnect(5)) {
                std::cerr << "[BOT] Failed to reconnect, exiting" << std::endl;
                break;  // break instead of return — ensures destructors are called
            }
            last_alert = time(NULL);  // Reset alert timer after reconnect
        }
    }

    g_bot = NULL;
    std::cout << "[BOT] Shutdown complete" << std::endl;
    return 0;
}