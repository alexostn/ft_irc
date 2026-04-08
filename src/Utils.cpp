/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sarherna <sarherna@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 14:00:00 by sarherna          #+#    #+#             */
/*   Updated: 2026/02/07 16:00:00 by sarherna         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Utils implementation
// Utility functions for string manipulation, validation, formatting

#include "irc/Utils.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <ctime>

std::string Utils::join(const std::vector<std::string>& strings, const std::string& delimiter)
{
    if (strings.empty())
        return "";
    
    std::string result = strings[0];
    for (size_t i = 1; i < strings.size(); ++i)
    {
        result += delimiter;
        result += strings[i];
    }
    return result;
}

// Remove leading and trailing whitespace
std::string Utils::trim(const std::string& str) {
    if (str.empty())
        return str;
    
    size_t start = 0;
    size_t end = str.length();
    
    // Find first non-whitespace
    while (start < end && std::isspace(static_cast<unsigned char>(str[start])))
        ++start;
    
    // Find last non-whitespace
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1])))
        --end;
    
    return str.substr(start, end - start);
}

// Convert string to uppercase
std::string Utils::toUpper(const std::string& str) {
    std::string result = str;
    for (size_t i = 0; i < result.length(); ++i) {
        result[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[i])));
    }
    return result;
}

// Convert string to lowercase
std::string Utils::toLower(const std::string& str) {
    std::string result = str;
    for (size_t i = 0; i < result.length(); ++i) {
        result[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
    }
    return result;
}

// Split string by delimiter
std::vector<std::string> Utils::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

// Validate nickname (SIMPLIFIED rules per TEAM_CONVENTIONS.md)
// Length: 1-9 characters
// First char: letter or underscore
// Rest: letters, digits, underscore
bool Utils::isValidNickname(const std::string& nickname) {
    if (nickname.empty() || nickname.length() > 9)
        return false;
    
    // First char must be letter or underscore
    char first = nickname[0];
    if (!std::isalpha(static_cast<unsigned char>(first)) && first != '_')
        return false;
    
    // Rest must be letters, digits, or underscore
    for (size_t i = 1; i < nickname.length(); ++i) {
        char c = nickname[i];
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
            return false;
    }
    
    return true;
}

// Validate channel name (SIMPLIFIED rules per TEAM_CONVENTIONS.md)
// Must start with # (only # channels, no & local channels)
// Length: 2-50 characters (including #)
// Cannot contain: space, comma, or control characters (0x00-0x1F)
bool Utils::isValidChannelName(const std::string& channelName) {
    if (channelName.length() < 2 || channelName.length() > 50)
        return false;
    
    if (channelName[0] != '#')
        return false;
    
    for (size_t i = 1; i < channelName.length(); ++i) {
        char c = channelName[i];
        if (c == ' ' || c == ',' || c < 0x20)
            return false;
    }
    
    return true;
}

// Check if name is a channel name (starts with #)
bool Utils::isChannelName(const std::string& name) {
    return !name.empty() && name[0] == '#';
}

// Convert string to integer (C++98 compatible)
int Utils::stringToInt(const std::string& str) {
    std::stringstream ss(str);
    int result = 0;
    ss >> result;
    return result;
}

// Convert integer to string (C++98 compatible)
std::string Utils::intToString(int value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

// Get current time as formatted string
std::string Utils::getCurrentTime() {
    time_t now = std::time(NULL);
    struct tm* timeinfo = std::localtime(&now);
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return std::string(buffer);
}

// Get current time as time_t
time_t Utils::getCurrentTimestamp() {
    return std::time(NULL);
}
