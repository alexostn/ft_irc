/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Topic.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sarherna <sarherna@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 14:00:00 by sarherna          #+#    #+#             */
/*   Updated: 2026/02/07 16:00:00 by sarherna         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// TOPIC command handler
// Sets or gets channel topic
// Format: TOPIC <channel> [<topic>]
// Follows TEAM_CONVENTIONS.md for Halloy compatibility

#include "irc/Server.hpp"
#include "irc/Client.hpp"
#include "irc/Channel.hpp"
#include "irc/Command.hpp"
#include "irc/Replies.hpp"
#include "irc/commands/Topic.hpp"

// Helper to get param safely
static std::string getParam(const Command& cmd, size_t index) {
    if (index < cmd.params.size()) {
        return cmd.params[index];
    }
    return "";
}

void handleTopic(Server& server, Client& client, const Command& cmd) {
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
            Replies::ERR_NEEDMOREPARAMS, nick, "TOPIC",
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
    
    // 4. Check if client is in channel
    if (!channel->hasClient(&client)) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NOTONCHANNEL, nick, channel->getNameDisplay(),
            "You're not on that channel"));
        return;
    }
    
    // 5. Query vs. Set topic
    if (cmd.trailing.empty() && cmd.params.size() == 1) {
        // Query topic
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
        return;
    }
    
    // Setting topic - check permissions
    // Check +t mode (topic protected - only ops can change)
    if (channel->hasMode('t') && !channel->isOperator(&client)) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_CHANOPRIVSNEEDED, nick, channel->getNameDisplay(),
            "You're not channel operator"));
        return;
    }
    
    // Set new topic
    std::string newTopic = cmd.trailing;
    channel->setTopic(newTopic, client.getNicknameDisplay());
    
    // Broadcast TOPIC change to all members
    std::string topicMsg = ":" + client.getPrefix() + " TOPIC " + 
                          channel->getNameDisplay() + " :" + 
                          newTopic + "\r\n";
    
    server.sendToClient(fd, topicMsg);
    channel->broadcast(&server, topicMsg, &client);
}
