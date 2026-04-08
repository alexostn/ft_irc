/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Pass.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sarherna <sarherna@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 14:00:00 by sarherna          #+#    #+#             */
/*   Updated: 2026/02/07 16:00:00 by sarherna         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// PASS command handler
// Sets password for client connection
// Format: PASS <password>
// Follows TEAM_CONVENTIONS.md for Halloy compatibility

#include "irc/Server.hpp"
#include "irc/Client.hpp"
#include "irc/Command.hpp"
#include "irc/Replies.hpp"
#include "irc/commands/Pass.hpp"

// Helper to get param safely (returns empty string if out of bounds)
static std::string getParam(const Command& cmd, size_t index) {
    if (index < cmd.params.size()) {
        return cmd.params[index];
    }
    return "";
}

void handlePass(Server& server, Client& client, const Command& cmd) {
    int fd = client.getFd();
    
    // Get current nick for error messages (use "*" if not set)
    std::string nick = client.getNicknameDisplay();
    if (nick.empty()) {
        nick = "*";
    }
    
    // 1. Already past PASS stage? (step > 1 means NICK already done)
    if (client.getRegistrationStep() > 1) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_ALREADYREGISTRED,
            nick,
            "",
            "You may not reregister"));
        return;
    }
    
    // 2. Missing or empty parameter?
    std::string password = getParam(cmd, 0);
    if (cmd.params.empty() || password.empty()) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NEEDMOREPARAMS,
            nick,
            "PASS",
            "Not enough parameters"));
        return;
    }
    
    // 3. Check password against server password
    if (password != server.getPassword()) {
        client.incrementPasswordAttempts();
        
        // Halloy compatibility: allow up to 3 attempts
        if (client.hasExceededPasswordAttempts()) {
            // Max attempts exceeded - disconnect
            server.sendToClient(fd, Replies::numeric(
                Replies::ERR_PASSWDMISMATCH,
                nick,
                "",
                "Password incorrect (max attempts exceeded)"));
            server.disconnectClient(fd);
        } else {
            // Allow retry
            server.sendToClient(fd, Replies::numeric(
                Replies::ERR_PASSWDMISMATCH,
                nick,
                "",
                "Password incorrect"));
        }
        return;
    }
    
    // 4. Success - move from step 0 to step 1
    client.setPassword();
    // No response needed on success - client proceeds to NICK
}
