/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CommandRegistry.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akacprzy <akacprzy@student.42warsaw.pl>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/26 23:22:39 by akacprzy          #+#    #+#             */
/*   Updated: 2026/01/26 23:24:41 by akacprzy         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef COMMANDREGISTRY_HPP

# define COMMANDREGISTRY_HPP

# include <string>
# include <map>
# include "irc/Command.hpp"

// Forward declarations
class Server;
class Client;

// Command handler function type
typedef void (*CommandHandler)(Server& server, Client& client, const Command& cmd);

// CommandRegistry class - manages command handlers and routes commands
// Maps command names to their handler functions
class CommandRegistry {
public:
    // Constructor: register all command handlers
    CommandRegistry();
    
    // Destructor
    ~CommandRegistry();
    
    // Execute a command (find handler and call it)
    // Returns true if command was found and executed, false otherwise
    bool execute(Server& server, Client& client, const Command& cmd);
    
    // Register a command handler
    void registerCommand(const std::string& command, CommandHandler handler);
    
    // Check if a command is registered
    bool hasCommand(const std::string& command) const;
    
private:
    std::map<std::string, CommandHandler> handlers_;
    
    // Initialize all command handlers
    void initializeHandlers();
};

#endif
