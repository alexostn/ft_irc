// channel_flood_bot.cpp — IRC bot (ft_irc bonus part)
// Compile: g++ -Wall -Wextra -Werror -std=c++98 -o channel_flood_bot src/channel_flood_bot.cpp
// Usage:   ./channel_flood_bot [host] [port] [pass] [nick] [channel]

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>   // strerror
#include <cstdlib>   // atoi, exit
#include <cstdio>    // perror
#include <cerrno>    // errno
#include <ctime>     // time, ctime, difftime
#include <unistd.h>  // sleep, close
#include <netdb.h>   // gethostbyname
#include <csignal>   // sigaction, sigemptyset
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Global running flag — signal-safe type, set by signal_handler
static volatile sig_atomic_t g_running = 1;

// Graceful shutdown on SIGINT/SIGTERM/SIGQUIT — NOT registered for SIGTSTP (Ctrl+Z)
static void signal_handler(int sig)
{
    (void)sig;
    g_running = 0;
}

// ── send one IRC line terminated with \r\n ────────────────────────────────────

static void send_line(int fd, const std::string& line)
{
    std::string msg = line + "\r\n";
    if (send(fd, msg.c_str(), msg.size(), 0) < 0) {
        if (errno != EPIPE && errno != ECONNRESET)
            std::cerr << "[ERR] send: " << strerror(errno) << std::endl;
        g_running = 0;  // trigger clean exit instead of exit()
        return;
    }
    std::cout << "[BOT>>] " << line << std::endl;
}

// ── append chunk to buffer, extract complete \r\n lines ──────────────────────

static std::vector<std::string> extract_lines(std::string& buf,
                                               const char*  chunk,
                                               ssize_t      n)
{
    buf.append(chunk, n);
    std::vector<std::string> lines;
    size_t pos;
    while ((pos = buf.find("\r\n")) != std::string::npos) {
        lines.push_back(buf.substr(0, pos));
        buf.erase(0, pos + 2);
    }
    return lines;
}

// ── parse :nick!user@host PRIVMSG target :text ───────────────────────────────

static bool parse_privmsg(const std::string& line,
                           std::string&       sender,
                           std::string&       target,
                           std::string&       text)
{
    if (line.empty() || line[0] != ':') return false;

    size_t ex  = line.find('!');
    size_t sp1 = line.find(' ');
    size_t sp2 = (sp1 != std::string::npos) ? line.find(' ', sp1 + 1) : std::string::npos;
    size_t sp3 = (sp2 != std::string::npos) ? line.find(' ', sp2 + 1) : std::string::npos;
    size_t col = line.find(':', sp3);   // skip colons inside IPv6 hostmask

    if (ex  == std::string::npos || sp1 == std::string::npos ||
        sp2 == std::string::npos || sp3 == std::string::npos)
        return false;

    if (line.substr(sp1 + 1, sp2 - sp1 - 1) != "PRIVMSG")
        return false;

    sender = line.substr(1, ex - 1);
    target = line.substr(sp2 + 1, sp3 - sp2 - 1);
    text   = (col != std::string::npos) ? line.substr(col + 1) : "";

    if (!text.empty() && text[text.size() - 1] == '\r')  // strip trailing \r
        text.erase(text.size() - 1);

    return true;
}

// ── respond to recognised bot commands ───────────────────────────────────────

static void handle_command(int                fd,
                            const std::string& sender,
                            const std::string& target,
                            const std::string& text,
                            time_t             start_time)
{
    // reply to channel; if PM — reply directly to sender
    std::string reply = (!target.empty() && target[0] == '#') ? target : sender;

    if (text == "!ping") {
        send_line(fd, "PRIVMSG " + reply + " :pong!");

    } else if (text == "!help") {
        send_line(fd, "PRIVMSG " + reply
            + " :commands: !ping  !echo <text>  !time  !uptime  !help");

    } else if (text.size() > 6 && text.substr(0, 6) == "!echo ") {
        send_line(fd, "PRIVMSG " + reply + " :" + text.substr(6));

    } else if (text == "!time") {
        time_t      t  = time(NULL);
        std::string ts = ctime(&t);
        // strip trailing newline added by ctime()
        if (!ts.empty() && ts[ts.size() - 1] == '\n')
            ts.erase(ts.size() - 1);
        send_line(fd, "PRIVMSG " + reply + " :" + ts);

    } else if (text == "!uptime") {
        long secs = static_cast<long>(difftime(time(NULL), start_time));
        std::ostringstream oss;
        oss << "uptime: "
            << secs / 3600         << "h "
            << (secs % 3600) / 60  << "m "
            << secs % 60           << "s";
        send_line(fd, "PRIVMSG " + reply + " :" + oss.str());
    }
    // unknown commands are silently ignored — don't spam the channel
}

// ── entry point ───────────────────────────────────────────────────────────────

int main(int argc, char* argv[])
{
    const char* host    = (argc > 1) ? argv[1] : "127.0.0.1";
    int         port    = (argc > 2) ? atoi(argv[2]) : 6667;
    const char* pass    = (argc > 3) ? argv[3] : "testpass";
    const char* nick    = (argc > 4) ? argv[4] : "flood_bot";
    const char* channel = (argc > 5) ? argv[5] : "#test";

    // SIGINT (Ctrl+C), SIGTERM, SIGQUIT — graceful shutdown
    // SA_RESTART: select() auto-restarts after signal — no manual EINTR retry needed
    // SIGTSTP (Ctrl+Z) intentionally NOT handled — freeze/thaw test must work
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);

    // SIGPIPE — ignore broken pipe on send() to disconnected server
    struct sigaction sa_pipe;
    sa_pipe.sa_handler = SIG_IGN;
    sigemptyset(&sa_pipe.sa_mask);
    sa_pipe.sa_flags = 0;
    sigaction(SIGPIPE, &sa_pipe, NULL);

    struct hostent* he = gethostbyname(host);
    if (!he) { std::cerr << "[ERR] gethostbyname failed\n"; return 1; }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect"); close(fd); return 1;
    }
    std::cout << "[*] connected to " << host << ":" << port << std::endl;

    // IRC registration handshake
    send_line(fd, std::string("PASS ") + pass);
    send_line(fd, std::string("NICK ") + nick);
    send_line(fd, std::string("USER ") + nick + " 0 * :IRC Bot");

    sleep(1);   // wait for 001-004 welcome numerics
    send_line(fd, std::string("JOIN ") + channel);
    std::cout << "[*] joined " << channel << " — listening\n";

    time_t      start_time = time(NULL);
    std::string recv_buf;
    char        tmp[4096];

    while (g_running)
    {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        // SA_RESTART handles EINTR automatically — only real errors reach here
        if (select(fd + 1, &rfds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;   // defensive: should not reach with SA_RESTART
            perror("select"); break;
        }

        // Check g_running after select() unblocks — signal may have fired
        if (!g_running)
            break;

        ssize_t n = recv(fd, tmp, sizeof(tmp) - 1, 0);
        if (n <= 0) {
            std::cerr << "[*] server closed connection\n"; break;
        }

        std::vector<std::string> lines = extract_lines(recv_buf, tmp, n);
        for (size_t i = 0; i < lines.size(); ++i)
        {
            const std::string& line = lines[i];
            std::cout << "[<<SVR] " << line << std::endl;

            // must reply to PING or server will disconnect after ~120s
            if (line.size() >= 4 && line.substr(0, 4) == "PING") {
                send_line(fd, "PONG" + line.substr(4));
                continue;
            }

            std::string sender, target, text;
            if (parse_privmsg(line, sender, target, text))
                handle_command(fd, sender, target, text, start_time);
        }
    }

    // Explicit cleanup — guarantees valgrind: definitely lost: 0 bytes
    std::string().swap(recv_buf);   // force recv_buf heap deallocation
    close(fd);
    std::cout << "[*] shutdown complete\n";
    return 0;
}
