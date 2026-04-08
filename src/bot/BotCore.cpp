// BotCore.cpp — IRC connection, registration, poll(), reconnect logic

#include "irc/bot/BotCore.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

BotCore::BotCore(const std::string& host, int port, const std::string& pass,
                  const std::string& nick, const std::string& channel)
    : host_(host), port_(port), pass_(pass), nick_(nick),
      channel_(channel), fd_(-1), running_(true)
{
}

BotCore::~BotCore()
{
    std::string().swap(recv_buffer_);  // Force buffer deallocation
    close();
}

bool BotCore::connectToServer()
{
    struct hostent* host_ent = gethostbyname(host_.c_str());
    if (!host_ent) {
        std::cerr << "[ERR] gethostbyname(" << host_ << ") failed" << std::endl;
        return false;
    }

    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0) {
        std::cerr << "[ERR] socket: " << strerror(errno) << std::endl;
        return false;
    }

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr = *((struct in_addr*)host_ent->h_addr);

    if (::connect(fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[ERR] connect: " << strerror(errno) << std::endl;
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    // Non-blocking socket
    int flags = fcntl(fd_, F_GETFL, 0);
    if (flags < 0 || fcntl(fd_, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::cerr << "[ERR] fcntl: " << strerror(errno) << std::endl;
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    std::cout << "[BOT] Connected to " << host_ << ":" << port_ << std::endl;
    return true;
}

bool BotCore::connect()
{
    return connectToServer();
}

bool BotCore::registerIRC()
{
    if (fd_ < 0)
        return false;

    sendRaw("PASS " + pass_);
    sendRaw("NICK " + nick_);
    sendRaw("USER " + nick_ + " 0 * :" + nick_);

    sleep(1);  // Allow server to process registration

    sendRaw("JOIN " + channel_);
    return true;
}

void BotCore::sendRaw(const std::string& msg)
{
    if (fd_ < 0)
        return;

    std::string line = msg + "\r\n";
    ssize_t n = send(fd_, line.c_str(), line.size(), 0);
    if (n < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "[ERR] send: " << strerror(errno) << std::endl;
            ::close(fd_);
            fd_ = -1;
        }
    } else {
        std::cout << "[BOT>>] " << msg << std::endl;
    }
}

std::vector<std::string> BotCore::extractLines()
{
    std::vector<std::string> lines;
    char buf[4096];
    ssize_t n = recv(fd_, buf, sizeof(buf) - 1, 0);

    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return lines;  // No data available
        } else {
            std::cerr << "[ERR] recv: " << strerror(errno) << std::endl;
            ::close(fd_);
            fd_ = -1;
            return lines;
        }
    } else if (n == 0) {
        std::cout << "[BOT] Server closed connection" << std::endl;
        ::close(fd_);
        fd_ = -1;
        return lines;
    }

    buf[n] = '\0';
    recv_buffer_.append(buf, n);

    size_t pos;
    while ((pos = recv_buffer_.find("\r\n")) != std::string::npos) {
        lines.push_back(recv_buffer_.substr(0, pos));
        recv_buffer_.erase(0, pos + 2);
    }

    return lines;
}

void BotCore::handlePing(const std::string& line)
{
    if (line.size() > 5 && line.substr(0, 5) == "PING ") {
        std::string server = line.substr(5);
        sendRaw("PONG " + server);
    }
}

std::vector<std::string> BotCore::tick(int timeout_ms)
{
    std::vector<std::string> result;

    if (fd_ < 0)
        return result;

    struct pollfd pfd;
    pfd.fd = fd_;
    pfd.events = POLLIN;
    pfd.revents = 0;

    int poll_ret = poll(&pfd, 1, timeout_ms);

    if (poll_ret < 0) {
        std::cerr << "[ERR] poll: " << strerror(errno) << std::endl;
        ::close(fd_);
        fd_ = -1;
        return result;
    }

    if (poll_ret > 0 && (pfd.revents & POLLIN)) {
        result = extractLines();
        for (size_t i = 0; i < result.size(); ++i) {
            handlePing(result[i]);
        }
    }

    return result;
}

bool BotCore::reconnect(int max_attempts)
{
    for (int attempt = 1; attempt <= max_attempts; ++attempt) {
        std::cout << "[BOT] Reconnect attempt " << attempt << "/" << max_attempts
                  << std::endl;
        sleep(3);

        if (connect() && registerIRC()) {
            std::cout << "[BOT] Reconnected successfully" << std::endl;
            return true;
        }
    }

    std::cout << "[BOT] Failed to reconnect after " << max_attempts
              << " attempts" << std::endl;
    return false;
}

bool BotCore::isConnected() const
{
    return fd_ >= 0;
}

void BotCore::close()
{
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
        std::cout << "[BOT] Connection closed" << std::endl;
    }
}

void BotCore::stop()
{
    running_ = false;
}

bool BotCore::isRunning() const
{
    return running_;
}
