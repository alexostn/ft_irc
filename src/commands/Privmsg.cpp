/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Privmsg.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sarherna <sarherna@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 14:00:00 by sarherna          #+#    #+#             */
/*   Updated: 2026/02/07 16:00:00 by sarherna         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// PRIVMSG command handler
// Sends a message to a channel or user
// Format: PRIVMSG <target>{,<target>} :<message>
// Follows TEAM_CONVENTIONS.md for Halloy compatibility

#include "irc/Server.hpp"
#include "irc/Client.hpp"
#include "irc/Channel.hpp"
#include "irc/Command.hpp"
#include "irc/Replies.hpp"
#include "irc/Utils.hpp"
#include "irc/commands/Privmsg.hpp"

// Helper to get param safely
static std::string getParam(const Command& cmd, size_t index) {
    if (index < cmd.params.size()) {
        return cmd.params[index];
    }
    return "";
}

void handlePrivmsg(Server& server, Client& client, const Command& cmd) {
    int fd = client.getFd();
    std::string nick = client.getNicknameDisplay();
    
    // 1. Check if client is registered
    if (!client.isRegistered()) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NOTREGISTERED, "*", "",
            "You have not registered"));
        return;
    }
    
    // 2. Check if target and message are provided
    if (cmd.params.empty()) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NEEDMOREPARAMS, nick, "PRIVMSG",
            "Not enough parameters"));
        return;
    }
    
    std::string target = getParam(cmd, 0);
    // Message: trailing (after ':') or second param (Halloy sends PRIVMSG #chan msg without ':')
    std::string message = cmd.trailing;
    if (message.empty()) {
        message = getParam(cmd, 1);
    }
    if (message.empty()) {
        // ERR_NOTEXTTOSEND (412)
        server.sendToClient(fd, ":server 412 " + nick +
                           " :No text to send\r\n");
        return;
    }
    
    // 3. Determine target type: channel or user
    if (Utils::isChannelName(target)) {
        // Channel message
        Channel* channel = server.getChannel(target);
        
        if (!channel) {
            server.sendToClient(fd, Replies::numeric(
                Replies::ERR_NOSUCHCHANNEL, nick, target,
                "No such channel"));
            return;
        }
        
        if (!channel->hasClient(&client)) {
            server.sendToClient(fd, Replies::numeric(
                Replies::ERR_CANNOTSENDTOCHAN, nick, channel->getNameDisplay(),
                "Cannot send to channel"));
            return;
        }
        
        // Format and broadcast message
        std::string msg = ":" + client.getPrefix() + " PRIVMSG " + 
                          channel->getNameDisplay() + " :" + message + "\r\n";
        
        // Broadcast to channel (excluding sender)
        channel->broadcast(&server, msg, &client);
        
    } else {
        // Private message to user
        Client* targetClient = server.getClientByNickname(target);
        
        if (!targetClient) {
            server.sendToClient(fd, Replies::numeric(
                Replies::ERR_NOSUCHNICK, nick, target,
                "No such nick/channel"));
            return;
        }
        
        // Format and send message
        std::string msg = ":" + client.getPrefix() + " PRIVMSG " + 
                          targetClient->getNicknameDisplay() + " :" + 
                          message + "\r\n";
        
        server.sendToClient(targetClient->getFd(), msg);
    }
}
