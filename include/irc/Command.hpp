/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Command.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akacprzy <akacprzy@student.42warsaw.pl>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/26 22:54:15 by akacprzy          #+#    #+#             */
/*   Updated: 2026/01/26 22:55:07 by akacprzy         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef COMMAND_HPP

# define COMMAND_HPP

# include <string>
# include <vector>

// Command structure - parsed IRC message
// This is the format used to pass parsed messages between modules
struct Command {
    // Optional origin prefix (e.g., "nick!user@host")
    std::string prefix;
    
    // Command name (e.g., "NICK", "PRIVMSG", "JOIN")
    std::string command;
    
    // Command parameters (space-separated, before trailing)
    std::vector<std::string> params;
    
    // Trailing parameter (content after ':')
    std::string trailing;
    
    // Original unparsed message (for debugging/logging)
    std::string raw;
    
    // Helper methods (can be implemented as free functions or in Utils)
    // Can be removed if they won't get to be used
    // bool hasPrefix() const { return !prefix.empty(); }
    // std::string getParam(size_t index) const;
    // size_t paramCount() const { return params.size(); }
    // std::string getFullCommand() const;  // Reconstruct command string
};

#endif
