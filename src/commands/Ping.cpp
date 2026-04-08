/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Ping.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sarherna <sarherna@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 14:00:00 by sarherna          #+#    #+#             */
/*   Updated: 2026/02/07 16:00:00 by sarherna         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// PING command handler
// Server sends PING to check if client is alive
// Client responds with PONG
// Format: PING <server1> [<server2>]
// Follows TEAM_CONVENTIONS.md for Halloy compatibility

#include "irc/Server.hpp"
#include "irc/Client.hpp"
#include "irc/Command.hpp"
#include "irc/Replies.hpp"
#include "irc/commands/Ping.hpp"

// Helper to get param safely
static std::string getParam(const Command& cmd, size_t index) {
    if (index < cmd.params.size()) {
        return cmd.params[index];
    }
    return "";
}

void handlePing(Server& server, Client& client, const Command& cmd) {
    int fd = client.getFd();
    
    // Get token from params or trailing
    std::string token = cmd.params.empty() ? cmd.trailing : getParam(cmd, 0);
    
    if (token.empty()) {
        std::string nick = client.getNicknameDisplay();
        if (nick.empty()) nick = "*";
        
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NEEDMOREPARAMS, nick, "PING",
            "Not enough parameters"));
        return;
    }
    
    // Send PONG with same token
    std::string pongMsg = ":" + Replies::formatServerName() + 
                          " PONG " + Replies::formatServerName() + 
                          " :" + token + "\r\n";
    
    server.sendToClient(fd, pongMsg);
}
