/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akacprzy <akacprzy@student.42warsaw.pl>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/19 23:21:22 by akacprzy          #+#    #+#             */
/*   Updated: 2026/02/08 14:09:46 by akacprzy         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "irc/Parser.hpp"
#include "irc/Command.hpp"
#include "irc/Utils.hpp"

// Constructor
Parser::Parser() {}

// Deconstructor
Parser::~Parser() {}

// Method - parse() - main parsing logic:
bool Parser::parse(const std::string& message, Command& cmd)
{
	std::string line = message;
	size_t pos = 0;

    // 1. Reset command structure
    cmd.raw = message;
    cmd.prefix = "";
    cmd.command = "";
    cmd.params.clear();
    cmd.trailing = "";

    // 2. Remove trailing \r\n
    if (line.length() > 1 && line[line.length() - 2] == '\r' && line[line.length() - 1] == '\n')
        line.erase(line.length() - 2);
    else if (!line.empty() && line[line.length() - 1] == '\n')
        line.erase(line.length() - 1);

    if (line.empty()) return false;

    // Skip leading spaces
    while (pos < line.length() && line[pos] == ' ')
        pos++;
    if (pos >= line.length()) return false;

    // 3. Prefix (starts with :)
    if (line[pos] == ':')
	{
        size_t end = line.find(' ', pos);
		// Invalid: Prefix but no command
        if (end == std::string::npos)
			return false;
        cmd.prefix = line.substr(pos + 1, end - pos - 1);
        pos = end + 1;
        // Skip spaces
        while (pos < line.length() && line[pos] == ' ')
			pos++;
    }

    // 4. Command
	// Invalid: No command
    if (pos >= line.length())
		return false;
    
    size_t end = line.find(' ', pos);
    if (end == std::string::npos) 
	{
        cmd.command = line.substr(pos);
        pos = line.length();
    }
	else
	{
        cmd.command = line.substr(pos, end - pos);
        pos = end + 1;
    }
    
    // Normalize command to uppercase
    cmd.command = Utils::toUpper(cmd.command);


    // 5. Params & Trailing
    while (pos < line.length())
	{
        // Check for trailing parameter (starts with :)
        if (line[pos] == ':')
		{
            cmd.trailing = line.substr(pos + 1);
            break; // Trailing consumes the rest of the line
        }
        
        // Regular parameter
        end = line.find(' ', pos);
        if (end == std::string::npos)
		{
            cmd.params.push_back(line.substr(pos));
            break;
        }
        cmd.params.push_back(line.substr(pos, end - pos));
        pos = end + 1;
    }

    return true;
}
