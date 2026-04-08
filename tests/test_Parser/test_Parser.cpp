// How to run test: from main directory run following 2 lines of code:
// c++ -Wall -Wextra -Werror -std=c++98 -I include src/Parser.cpp src/Utils.cpp tests/test_Parser/test_Parser.cpp -o tests/test_Parser/run_test_Parser
// ./tests/test_Parser/run_test_Parser

#include <iostream>
#include <cassert>
#include <vector>
#include "irc/Parser.hpp"
#include "irc/Command.hpp"

void printPass(const std::string& testName)
{
    std::cout << "[PASS] " << testName << std::endl;
}

void test_simple_command()
{
    Parser parser;
    Command cmd;
    
    // Act
    bool result = parser.parse("NICK user\r\n", cmd);
    
    // Assert
    assert(result == true);
    assert(cmd.command == "NICK");
    assert(cmd.params.size() == 1);
    assert(cmd.params[0] == "user");
    assert(cmd.prefix.empty());
    assert(cmd.trailing.empty());
    
    printPass("Simple command (NICK user)");
}

void test_with_prefix()
{
    Parser parser;
    Command cmd;
    
    // Act
    bool result = parser.parse(":origin COMMAND param\r\n", cmd);
    
    // Assert
    assert(result == true);
    assert(cmd.prefix == "origin");
    assert(cmd.command == "COMMAND");
    assert(cmd.params.size() == 1);
    assert(cmd.params[0] == "param");
    
    printPass("Command with prefix");
}

void test_with_trailing()
{
    Parser parser;
    Command cmd;
    
    // Act
    bool result = parser.parse("PRIVMSG #channel :Hello World!\r\n", cmd);
    
    // Assert
    assert(result == true);
    assert(cmd.command == "PRIVMSG");
    assert(cmd.params.size() == 1);
    assert(cmd.params[0] == "#channel");
    assert(cmd.trailing == "Hello World!");
    
    printPass("Command with trailing parameter");
}

void test_case_insensitivity()
{
    Parser parser;
    Command cmd;
    
    // Act
    parser.parse("nick user\r\n", cmd);
    
    // Assert
    assert(cmd.command == "NICK");
    
    printPass("Command case insensitivity");
}

void test_empty_trailing()
{
    Parser parser;
    Command cmd;
    
    parser.parse("CMD param :\r\n", cmd);
    assert(cmd.trailing == "");
    printPass("Empty trailing parameter");
}

void test_whitespace_only()
{
    Parser parser;
    Command cmd;
    
    // Act
    bool result = parser.parse("   \r\n", cmd);
    
    // Assert
    assert(result == false);
    printPass("Whitespace only message ignored");
}

void test_newline_only()
{
    Parser parser;
    Command cmd;
    
    // Act
    bool result = parser.parse("\r\n", cmd);
    
    // Assert
    assert(result == false);
    printPass("Newline only message ignored");
}

void test_empty_params_halloy()
{
    Parser parser;
    Command cmd;
    
    // Act
    // "USER  0 * :Real Name" -> USER <empty> 0 * :Real Name
    bool result = parser.parse("USER   0  * :Real Name\r\n", cmd);
    
    // Assert
    assert(result == true);
    assert(cmd.command == "USER");
    assert(cmd.params.size() == 5);
    assert(cmd.params[0] == "");
    assert(cmd.params[1] == "");
    assert(cmd.params[2] == "0");
    assert(cmd.params[3] == "");
    assert(cmd.params[4] == "*");
    assert(cmd.trailing == "Real Name");
    
    printPass("Empty parameters (Halloy compatibility)");
}

int main()
{
    test_simple_command();
    test_with_prefix();
    test_with_trailing();
    test_case_insensitivity();
    test_empty_trailing();
    test_whitespace_only();
    test_newline_only();
    test_empty_params_halloy();
    std::cout << "\nAll Parser tests passed!" << std::endl;
    return 0;
}
