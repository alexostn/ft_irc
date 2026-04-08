/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Nick.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sarherna <sarherna@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 14:00:00 by sarherna          #+#    #+#             */
/*   Updated: 2026/02/07 16:00:00 by sarherna         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// NICK command handler
// Sets or changes client nickname
// Format: NICK <nickname>
// Follows TEAM_CONVENTIONS.md for Halloy compatibility

#include "irc/Server.hpp"
#include "irc/Client.hpp"
#include "irc/Channel.hpp"
#include "irc/Command.hpp"
#include "irc/Replies.hpp"
#include "irc/Utils.hpp"
#include "irc/commands/Nick.hpp"

// Helper to get param safely (returns empty string if out of bounds)
static std::string getParam(const Command& cmd, size_t index) {
    if (index < cmd.params.size()) {
        return cmd.params[index];
    }
    return "";
}

void handleNick(Server& server, Client& client, const Command& cmd) {
    int fd = client.getFd();
    
    // Get current nick for error messages (use "*" if not set)
    std::string currentNick = client.getNicknameDisplay();
    if (currentNick.empty()) {
        currentNick = "*";
    }
    
    // 1. Must have PASS first (if not already registered)
    // STRICT ORDER: PASS -> NICK -> USER
    if (!client.isRegistered() && client.getRegistrationStep() < 1) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NOTREGISTERED,
            currentNick,
            "",
            "You have not registered"));
        return;
    }
    
    // 2. Missing or empty parameter?
    std::string newNick = getParam(cmd, 0);
    if (cmd.params.empty() || newNick.empty()) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NONICKNAMEGIVEN,
            currentNick,
            "",
            "No nickname given"));
        return;
    }
    
    // 3. Validate nickname format (SIMPLIFIED rules)
    // Length: 1-9 chars, first char: letter/underscore, rest: letters/digits/underscore
    if (!Utils::isValidNickname(newNick)) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_ERRONEUSNICKNAME,
            currentNick,
            newNick,
            "Erroneous nickname"));
        return;
    }
    
    // 4. Check if nickname is already in use (case-insensitive)
    // getClientByNickname should use Utils::toLower internally
    Client* existing = server.getClientByNickname(newNick);
    if (existing && existing != &client) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NICKNAMEINUSE,
            currentNick,
            newNick,
            "Nickname is already in use"));
        return;
    }
    
    // 5. Save state for nick change broadcast
    std::string oldPrefix = client.getPrefix();
    bool wasRegistered = client.isRegistered();
    bool hadNick = client.hasNickname();
    
    // 6. Set new nickname (stores both lowercase and display version)
    client.setNickname(newNick);
    
    // 7. Advance registration if this is the first NICK and we're at step 1
    if (!wasRegistered && client.getRegistrationStep() == 1) {
        client.completeNickStep();  // step 1 -> 2
    }
    
    // 8. If user already had a nickname, broadcast nick change to all channels
    if (hadNick && wasRegistered) {
        std::string nickMsg = ":" + oldPrefix + " NICK :" + newNick + "\r\n";
        
        // Also send to the client itself
        server.sendToClient(fd, nickMsg);
        
        // Broadcast to all channels the user is in
        const std::vector<std::string>& channels = client.getChannels();
        for (size_t i = 0; i < channels.size(); ++i) {
            Channel* chan = server.getChannel(channels[i]);
            if (chan) {
                // broadcast() sends to all members except the excluded client
                chan->broadcast(&server, nickMsg, &client);
            }
        }
    }
}
