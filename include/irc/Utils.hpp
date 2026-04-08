/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sarherna <sarherna@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 14:00:00 by sarherna          #+#    #+#             */
/*   Updated: 2026/02/07 16:00:00 by sarherna         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <ctime>

// Utility functions for IRC server
// String manipulation, validation, formatting helpers
class Utils {
public:
    // Join a vector of strings into a single string separated by delimiter
    static std::string join(const std::vector<std::string>& strings, const std::string& delimiter);

    // String manipulation
    static std::string trim(const std::string& str);
    static std::string toUpper(const std::string& str);
    static std::string toLower(const std::string& str);
    
    // Split string by delimiter
    static std::vector<std::string> split(const std::string& str, char delimiter);
    
    // Validation (SIMPLIFIED rules per TEAM_CONVENTIONS.md)
    // Nickname: 1-9 chars, first char letter/underscore, rest letters/digits/underscore
    static bool isValidNickname(const std::string& nickname);
    // Channel: starts with #, 2-50 chars, no space/comma/control chars
    static bool isValidChannelName(const std::string& channelName);
    
    // IRC protocol helpers
    static bool isChannelName(const std::string& name);  // Starts with #
    
    // Number conversion (C++98 compatible)
    static int stringToInt(const std::string& str);
    static std::string intToString(int value);
    
    // Time utilities
    static std::string getCurrentTime();
    static time_t getCurrentTimestamp();
};

#endif // UTILS_HPP
