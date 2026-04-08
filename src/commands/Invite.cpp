/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Invite.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sarherna <sarherna@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 14:00:00 by sarherna          #+#    #+#             */
/*   Updated: 2026/02/07 16:00:00 by sarherna         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// INVITE command handler
// Invites a user to an invite-only channel
// Format: INVITE <nickname> <channel>
// Follows TEAM_CONVENTIONS.md for Halloy compatibility

#include "irc/Server.hpp"
#include "irc/Client.hpp"
#include "irc/Channel.hpp"
#include "irc/Command.hpp"
#include "irc/Replies.hpp"
#include "irc/commands/Invite.hpp"

// Helper to get param safely
static std::string getParam(const Command& cmd, size_t index) {
    if (index < cmd.params.size()) {
        return cmd.params[index];
    }
    return "";
}

void handleInvite(Server& server, Client& client, const Command& cmd) {
    int fd = client.getFd();
    std::string nick = client.getNicknameDisplay();
    
    // 1. Check if client is registered
    if (!client.isRegistered()) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NOTREGISTERED, "*", "",
            "You have not registered"));
        return;
    }
    
    // 2. Check if nickname and channel parameters are provided
    if (cmd.params.size() < 2) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NEEDMOREPARAMS, nick, "INVITE",
            "Not enough parameters"));
        return;
    }
    
    std::string targetNick = getParam(cmd, 0);
    std::string channelName = getParam(cmd, 1);
    
    // 3. Get target client (case-insensitive)
    Client* target = server.getClientByNickname(targetNick);
    if (!target) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NOSUCHNICK, nick, targetNick,
            "No such nick/channel"));
        return;
    }
    
    // 4. Get channel
    Channel* channel = server.getChannel(channelName);
    if (!channel) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NOSUCHCHANNEL, nick, channelName,
            "No such channel"));
        return;
    }
    
    // 5. Check if inviter is in channel
    if (!channel->hasClient(&client)) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NOTONCHANNEL, nick, channel->getNameDisplay(),
            "You're not on that channel"));
        return;
    }
    
    // 6. Check if target is already in channel
    if (channel->hasClient(target)) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_USERONCHANNEL, nick,
            target->getNicknameDisplay() + " " + channel->getNameDisplay(),
            "is already on channel"));
        return;
    }
    
    // 7. Check operator requirement (only for +i channels)
    if (channel->hasMode('i') && !channel->isOperator(&client)) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_CHANOPRIVSNEEDED, nick, channel->getNameDisplay(),
            "You're not channel operator"));
        return;
    }
    
    // 8. Add to invite list
    channel->addToInviteList(target->getNickname());
    
    // 9. Send RPL_INVITING (341) to inviter
    server.sendToClient(fd, Replies::numeric(
        Replies::RPL_INVITING, nick,
        target->getNicknameDisplay() + " " + channel->getNameDisplay(), ""));
    
    // 10. Send INVITE message to target
    std::string inviteMsg = ":" + client.getPrefix() + " INVITE " + 
                           target->getNicknameDisplay() + " :" + 
                           channel->getNameDisplay() + "\r\n";
    server.sendToClient(target->getFd(), inviteMsg);
}
