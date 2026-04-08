/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akacprzy <akacprzy@student.42warsaw.pl>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/19 23:20:04 by akacprzy          #+#    #+#             */
/*   Updated: 2026/01/19 23:22:04 by akacprzy         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PARSER_HPP

# define PARSER_HPP

# include <string>
# include <vector>

// Forward declaration
struct Command;

// Parser class - parses IRC message format into Command structure
// Format: [:<prefix>] <command> [params] [:<trailing>]\r\n
class Parser {
public:
    // Constructor
    Parser();
    
    // Destructor
    ~Parser();
    
    // Parse a complete IRC message string into Command structure
    // Returns true if parsing successful, false otherwise
    bool parse(const std::string& message, Command& cmd);
};

#endif
