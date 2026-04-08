/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Join.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sarherna <sarherna@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 14:00:00 by sarherna          #+#    #+#             */
/*   Updated: 2026/02/07 16:00:00 by sarherna         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// JOIN command handler
// Client joins a channel
// Format: JOIN <channel>{,<channel>} [<key>{,<key>}]
// Follows TEAM_CONVENTIONS.md for Halloy compatibility

#include "irc/Server.hpp"
#include "irc/Client.hpp"
#include "irc/Channel.hpp"
#include "irc/Command.hpp"
#include "irc/Replies.hpp"
#include "irc/Utils.hpp"
#include "irc/commands/Join.hpp"

// Helper to get param safely
static std::string getParam(const Command& cmd, size_t index) {
    if (index < cmd.params.size()) {
        return cmd.params[index];
    }
    return "";
}

void handleJoin(Server& server, Client& client, const Command& cmd) {
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
    if (cmd.params.empty() || getParam(cmd, 0).empty()) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NEEDMOREPARAMS, nick, "JOIN",
            "Not enough parameters"));
        return;
    }
    
    std::string channelName = getParam(cmd, 0);
    std::string key = getParam(cmd, 1);  // may be empty
    
    // 3. Validate channel name format
    if (!Utils::isValidChannelName(channelName)) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_BADCHANMASK, nick, channelName,
            "Bad Channel Mask"));
        return;
    }
    
    // 4. Get or create channel (case-insensitive)
    Channel* channel = server.getChannel(channelName);
    bool isNew = (channel == NULL);
    
    if (isNew) {
        channel = server.createChannel(channelName);
    }
    
    // 5. Check if already in channel
    if (channel->hasClient(&client)) {
        // Already in channel, silently ignore
        return;
    }
    
    // 6. Mode checks: +k (key), +i (invite), +l (limit)
    
    // +k: key required
    if (channel->hasMode('k')) {
        if (key != channel->getChannelKey()) {
            server.sendToClient(fd, Replies::numeric(
                Replies::ERR_BADCHANNELKEY, nick, channel->getNameDisplay(),
                "Cannot join channel (+k)"));
            return;
        }
    }
    
    // +i: invite only
    if (channel->hasMode('i')) {
        if (!channel->isInvited(client.getNickname())) {
            server.sendToClient(fd, Replies::numeric(
                Replies::ERR_INVITEONLYCHAN, nick, channel->getNameDisplay(),
                "Cannot join channel (+i)"));
            return;
        }
    }
    
    // +l: user limit
    if (channel->hasMode('l')) {
        if (channel->getClientCount() >= static_cast<size_t>(channel->getUserLimit())) {
            server.sendToClient(fd, Replies::numeric(
                Replies::ERR_CHANNELISFULL, nick, channel->getNameDisplay(),
                "Cannot join channel (+l)"));
            return;
        }
    }
    
    // 7. Add client to channel
    channel->addClient(&client);
    client.addChannel(channel->getName());
    
    // First member becomes operator
    if (isNew || channel->getClientCount() == 1) {
        channel->addOperator(&client);
    }
    
    // Remove from invite list after successful join
    if (channel->isInvited(client.getNickname())) {
        channel->removeFromInviteList(client.getNickname());
    }
    
    // 8. Send JOIN confirmation to all members (including joiner)
    std::string joinMsg = ":" + client.getPrefix() + " JOIN :" + 
                          channel->getNameDisplay() + "\r\n";
    
    server.sendToClient(fd, joinMsg);
    channel->broadcast(&server, joinMsg, &client);
    
    // 9. Send topic (if exists)
    if (channel->hasTopic()) {
        // RPL_TOPIC (332)
        server.sendToClient(fd, Replies::numeric(
            Replies::RPL_TOPIC, nick, channel->getNameDisplay(),
            channel->getTopic()));
    } else {
        // RPL_NOTOPIC (331)
        server.sendToClient(fd, Replies::numeric(
            Replies::RPL_NOTOPIC, nick, channel->getNameDisplay(),
            "No topic is set"));
    }
    
    // 10. Send NAMES list
    std::vector<Client*> members = channel->getClients();
    std::string namesList;
    
    for (size_t i = 0; i < members.size(); ++i) {
        if (channel->isOperator(members[i])) {
            namesList += "@";
        }
        namesList += members[i]->getNicknameDisplay();
        if (i < members.size() - 1) {
            namesList += " ";
        }
    }
    
    // RPL_NAMREPLY (353)
    server.sendToClient(fd, Replies::numeric(
        Replies::RPL_NAMREPLY, nick,
        "= " + channel->getNameDisplay(), namesList));
    
    // RPL_ENDOFNAMES (366)
    server.sendToClient(fd, Replies::numeric(
        Replies::RPL_ENDOFNAMES, nick, channel->getNameDisplay(),
        "End of /NAMES list"));
}
