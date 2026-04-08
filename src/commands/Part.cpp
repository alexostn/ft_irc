/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Part.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sarherna <sarherna@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 14:00:00 by sarherna          #+#    #+#             */
/*   Updated: 2026/02/07 16:00:00 by sarherna         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// PART command handler
// Client leaves a channel
// Format: PART <channel>{,<channel>} [<reason>]
// Follows TEAM_CONVENTIONS.md for Halloy compatibility

#include "irc/Server.hpp"
#include "irc/Client.hpp"
#include "irc/Channel.hpp"
#include "irc/Command.hpp"
#include "irc/Replies.hpp"
#include "irc/commands/Part.hpp"

// Helper to get param safely
static std::string getParam(const Command& cmd, size_t index) {
    if (index < cmd.params.size()) {
        return cmd.params[index];
    }
    return "";
}

void handlePart(Server& server, Client& client, const Command& cmd) {
    int fd = client.getFd();
    std::string nick = client.getNicknameDisplay();
    
    // 1. Check if client is registered
    if (!client.isRegistered()) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NOTREGISTERED, "*", "",
            "You have not registered"));
        return;
    }
    
    // 2. Check if channel parameter is provided
    if (cmd.params.empty()) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NEEDMOREPARAMS, nick, "PART",
            "Not enough parameters"));
        return;
    }
    
    std::string channelName = getParam(cmd, 0);
    
    // 3. Get channel
    Channel* channel = server.getChannel(channelName);
    
    if (!channel) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NOSUCHCHANNEL, nick, channelName,
            "No such channel"));
        return;
    }
    
    // 4. Check if client is in the channel
    if (!channel->hasClient(&client)) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NOTONCHANNEL, nick, channel->getNameDisplay(),
            "You're not on that channel"));
        return;
    }
    
    // 5. Build PART message
    std::string reason = cmd.trailing.empty() ? "Leaving" : cmd.trailing;
    std::string partMsg = ":" + client.getPrefix() + " PART " + 
                          channel->getNameDisplay() + " :" + reason + "\r\n";
    
    // 6. Broadcast PART to all members (including leaver)
    server.sendToClient(fd, partMsg);
    channel->broadcast(&server, partMsg, &client);
    
    // 7. Remove client from channel
    channel->removeClient(&client);
    client.removeChannel(channel->getName());
    
    // 8. If channel is empty, delete it
    if (channel->isEmpty()) {
        server.removeChannel(channel->getName());
    }
}
