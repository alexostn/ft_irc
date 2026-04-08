// How to run test: from main directory run following 2 lines of code:
// c++ -Wall -Wextra -Werror -std=c++98 -Itests/test_Replies/include -Iinclude src/Replies.cpp tests/test_Replies/test_Replies.cpp -o tests/test_Replies/run_test_Replies
// ./tests/test_Replies/run_test_Replies


#include <iostream>
#include <cassert>
#include <string>
#include "irc/Replies.hpp"
#include "irc/Client.hpp"
#include "irc/Config.hpp"

void printPass(const std::string& testName) {
    std::cout << "[PASS] " << testName << std::endl;
}

// --- Mock Client Implementation ---
// Implementation of the mock Client class defined in tests/test_Replies/include/irc/Client.hpp

Client::Client(int fd) : fd_(fd) {}
Client::~Client() {}

const std::string& Client::getNickname() const {
    static std::string val = "TestNick";
    return val;
}

const std::string& Client::getUsername() const {
    static std::string val = "TestUser";
    return val;
}

const std::string& Client::getHostname() const {
    static std::string val = "TestHost";
    return val;
}

std::string Client::getPrefix() const {
    return getNickname() + "!" + getUsername() + "@" + getHostname();
}
// ----------------------------------

// --- Mock Config Implementation ---
const std::string& Config::getServerName() {
    static std::string val = "ft_irc";
    return val;
}

void test_numeric_reply() {
    // Act
    std::string result = Replies::numeric("001", "Nick", "", "Welcome to IRC");
    
    // Assert
    // Expected: ":ft_irc 001 Nick :Welcome to IRC\r\n"
    std::string expected = ":ft_irc 001 Nick :Welcome to IRC\r\n";
    assert(result == expected);
    
    printPass("Numeric reply (001)");
}

void test_numeric_reply_with_params() {
    // Act
    std::string result = Replies::numeric("401", "Nick", "BadNick", "No such nick");
    
    // Assert
    // Expected: ":ft_irc 401 Nick BadNick :No such nick\r\n"
    std::string expected = ":ft_irc 401 Nick BadNick :No such nick\r\n";
    assert(result == expected);
    
    printPass("Numeric reply with params (401)");
}

void test_command_reply() {
    // Act
    std::string result = Replies::command("nick!user@host", "PRIVMSG", "#channel", "Hello World");
    
    // Assert
    std::string expected = ":nick!user@host PRIVMSG #channel :Hello World\r\n";
    assert(result == expected);
    
    printPass("Command reply");
}

void test_simple_command() {
    // Act
    std::string result = Replies::simple("PING", "server.com");
    
    // Assert
    std::string expected = "PING server.com\r\n";
    assert(result == expected);
    
    printPass("Simple command");
}

void test_format_client_prefix() {
    // Setup
    Client client(1); // Mock client
    
    // Act
    std::string result = Replies::formatClientPrefix(client);
    
    // Assert
    std::string expected = "TestNick!TestUser@TestHost";
    assert(result == expected);
    
    printPass("Format client prefix");
}

int main() {
    test_numeric_reply();
    test_numeric_reply_with_params();
    test_command_reply();
    test_simple_command();
    test_format_client_prefix();
    
    std::cout << "\nAll Replies tests passed!" << std::endl;
    return 0;
}