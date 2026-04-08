/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Kick.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sarherna <sarherna@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 14:00:00 by sarherna          #+#    #+#             */
/*   Updated: 2026/02/07 16:00:00 by sarherna         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// KICK command handler
// Removes a user from a channel (operator command)
// Format: KICK <channel> <user> [<reason>]
// Follows TEAM_CONVENTIONS.md for Halloy compatibility

#include "irc/Server.hpp"
#include "irc/Client.hpp"
#include "irc/Channel.hpp"
#include "irc/Command.hpp"
#include "irc/Replies.hpp"
#include "irc/commands/Kick.hpp"

// Helper to get param safely
static std::string getParam(const Command& cmd, size_t index) {
    if (index < cmd.params.size()) {
        return cmd.params[index];
    }
    return "";
}

void handleKick(Server& server, Client& client, const Command& cmd) {
    int fd = client.getFd();
    std::string nick = client.getNicknameDisplay();
    
    // 1. Check if client is registered
    if (!client.isRegistered()) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NOTREGISTERED, "*", "",
            "You have not registered"));
        return;
    }
    
    // 2. Check if channel and user parameters are provided
    if (cmd.params.size() < 2) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NEEDMOREPARAMS, nick, "KICK",
            "Not enough parameters"));
        return;
    }
    
    std::string channelName = getParam(cmd, 0);
    std::string targetNick = getParam(cmd, 1);
    std::string reason = cmd.trailing.empty() ? 
                        client.getNicknameDisplay() : cmd.trailing;
    
    // 3. Get channel
    Channel* channel = server.getChannel(channelName);
    if (!channel) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NOSUCHCHANNEL, nick, channelName,
            "No such channel"));
        return;
    }
    
    // 4. Check if kicker is in channel
    if (!channel->hasClient(&client)) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NOTONCHANNEL, nick, channel->getNameDisplay(),
            "You're not on that channel"));
        return;
    }
    
    // 5. Check if kicker is operator
    if (!channel->isOperator(&client)) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_CHANOPRIVSNEEDED, nick, channel->getNameDisplay(),
            "You're not channel operator"));
        return;
    }
    
    // 6. Get target client (case-insensitive)
    Client* target = server.getClientByNickname(targetNick);
    if (!target) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NOSUCHNICK, nick, targetNick,
            "No such nick/channel"));
        return;
    }
    
    // 7. Check if target is in channel
    if (!channel->hasClient(target)) {
        server.sendToClient(fd, ":server 441 " + nick + " " + 
                           target->getNicknameDisplay() + " " + 
                           channel->getNameDisplay() + 
                           " :They aren't on that channel\r\n");
        return;
    }
    
    // 8. Build KICK message
    std::string kickMsg = ":" + client.getPrefix() + " KICK " + 
                          channel->getNameDisplay() + " " + 
                          target->getNicknameDisplay() + " :" + 
                          reason + "\r\n";
    
    // 9. Broadcast KICK to all members (including kicked user)
    server.sendToClient(target->getFd(), kickMsg);
    channel->broadcast(&server, kickMsg, target);
    
    // 10. Remove target from channel
    channel->removeClient(target);
    target->removeChannel(channel->getName());
    
    // 11. If channel is empty, delete it
    if (channel->isEmpty()) {
        server.removeChannel(channel->getName());
    }
}
