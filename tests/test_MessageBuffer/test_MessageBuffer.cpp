// How to run test: from main directory run following 2 lines of code:
// c++ -Wall -Wextra -Werror -std=c++98 -I include src/MessageBuffer.cpp tests/test_MessageBuffer/test_MessageBuffer.cpp -o tests/test_MessageBuffer/run_test_MessageBuffer
// ./tests/test_MessageBuffer/run_test_MessageBuffer

#include <iostream>
#include <cassert>
#include <vector>
#include "irc/MessageBuffer.hpp"

// Helper to print success
void printPass(const std::string& testName)
{
    std::cout << "[PASS] " << testName << std::endl;
}

void test_simple_message()
{
    MessageBuffer buf;
    
    // Act
    buf.append("NICK user\r\n");
    std::vector<std::string> msgs = buf.extractMessages();
    
    // Assert
    assert(msgs.size() == 1);
    assert(msgs[0] == "NICK user\r\n");
    assert(buf.isEmpty());
    
    printPass("Simple complete message");
}

void test_partial_message()
{
    MessageBuffer buf;
    
    // Act 1: Send partial data
    buf.append("NICK ");
    std::vector<std::string> msgs = buf.extractMessages();
    
    // Assert 1: Should be empty
    assert(msgs.empty());
    assert(!buf.isEmpty()); // Buffer should hold "NICK "
    
    // Act 2: Send rest of data
    buf.append("user\r\n");
    msgs = buf.extractMessages();
    
    // Assert 2: Should have full message now
    assert(msgs.size() == 1);
    assert(msgs[0] == "NICK user\r\n");
    assert(buf.isEmpty());
    
    printPass("Partial message assembly");
}

void test_multiple_messages()
{
    MessageBuffer buf;
    
    // Act: Send two messages at once
    buf.append("NICK user\r\nUSER name * * :Real Name\r\n");
    std::vector<std::string> msgs = buf.extractMessages();
    
    // Assert
    assert(msgs.size() == 2);
    assert(msgs[0] == "NICK user\r\n");
    assert(msgs[1] == "USER name * * :Real Name\r\n");
    
    printPass("Multiple messages in one chunk");
}

void test_split_delimiter()
{
    MessageBuffer buf;
    
    // Act: Send message where \r and \n are split
    buf.append("COMMAND\r");
    assert(buf.extractMessages().empty()); // Should wait for \n
    
    buf.append("\n");
    std::vector<std::string> msgs = buf.extractMessages();
    
    assert(msgs.size() == 1);
    assert(msgs[0] == "COMMAND\r\n");
    
    printPass("Split CRLF delimiter");
}

int main()
{
    test_simple_message();
    test_partial_message();
    test_multiple_messages();
    test_split_delimiter();
    std::cout << "\nAll MessageBuffer tests passed!" << std::endl;
    return 0;
}
