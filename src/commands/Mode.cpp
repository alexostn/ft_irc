/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Mode.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sarherna <sarherna@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 14:00:00 by sarherna          #+#    #+#             */
/*   Updated: 2026/02/07 16:00:00 by sarherna         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// MODE command handler
// Sets or gets channel or user modes
// Format: MODE <target> [<modes> [<mode-parameters>]]
// SIMPLIFIED: Only ONE mode per command (TEAM_CONVENTIONS.md)
// Follows TEAM_CONVENTIONS.md for Halloy compatibility

#include "irc/Server.hpp"
#include "irc/Client.hpp"
#include "irc/Channel.hpp"
#include "irc/Command.hpp"
#include "irc/Replies.hpp"
#include "irc/Utils.hpp"
#include "irc/commands/Mode.hpp"

// Helper to get param safely
static std::string getParam(const Command& cmd, size_t index) {
    if (index < cmd.params.size()) {
        return cmd.params[index];
    }
    return "";
}

void handleMode(Server& server, Client& client, const Command& cmd) {
    int fd = client.getFd();
    std::string nick = client.getNicknameDisplay();
    
    // 1. Check if client is registered
    if (!client.isRegistered()) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NOTREGISTERED, "*", "",
            "You have not registered"));
        return;
    }
    
    // 2. Check if target parameter is provided
    if (cmd.params.empty()) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NEEDMOREPARAMS, nick, "MODE",
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
    
    // 5. Query vs. Set mode
    if (cmd.params.size() == 1) {
        // Query modes (+k / +l need parameter values in 324 per RFC)
        std::string modes = channel->getModeString();
        if (modes.empty()) {
            modes = "+";
        }
        std::string modeParams;
        if (channel->hasMode('k')) {
            modeParams += " " + channel->getChannelKey();
        }
        if (channel->hasMode('l')) {
            modeParams += " " + Utils::intToString(channel->getUserLimit());
        }
        // RPL_CHANNELMODEIS (324)
        server.sendToClient(fd, Replies::numeric(
            Replies::RPL_CHANNELMODEIS, nick,
            channel->getNameDisplay() + " " + modes + modeParams, ""));
        return;
    }
    
    // Setting mode - check operator status
    if (!channel->isOperator(&client)) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_CHANOPRIVSNEEDED, nick, channel->getNameDisplay(),
            "You're not channel operator"));
        return;
    }
    
    std::string modeStr = getParam(cmd, 1);
    if (modeStr.length() < 2) {
        return;  // Invalid format
    }
    
    char sign = modeStr[0];  // '+' or '-'
    char mode = modeStr[1];  // 'i', 't', 'k', 'o', 'l'
    
    if (sign != '+' && sign != '-') {
        return;  // Invalid sign
    }
    
    // 6. Apply mode based on type
    switch (mode) {
        case 'i':  // invite-only
        case 't':  // topic-protected
            channel->setMode(mode, sign == '+');
            break;
            
        case 'k':  // key
            if (sign == '+') {
                std::string key = getParam(cmd, 2);
                if (key.empty()) {
                    server.sendToClient(fd, Replies::numeric(
                        Replies::ERR_NEEDMOREPARAMS, nick, "MODE",
                        "Not enough parameters"));
                    return;
                }
                channel->setChannelKey(key);
                channel->setMode('k', true);
            } else {
                channel->setChannelKey("");
                channel->setMode('k', false);
            }
            break;
            
        case 'o':  // operator
            {
                std::string targetNick = getParam(cmd, 2);
                if (targetNick.empty()) {
                    server.sendToClient(fd, Replies::numeric(
                        Replies::ERR_NEEDMOREPARAMS, nick, "MODE",
                        "Not enough parameters"));
                    return;
                }
                
                Client* target = server.getClientByNickname(targetNick);
                if (!target) {
                    server.sendToClient(fd, Replies::numeric(
                        Replies::ERR_NOSUCHNICK, nick, targetNick,
                        "No such nick/channel"));
                    return;
                }
                
                if (!channel->hasClient(target)) {
                    server.sendToClient(fd, ":server 441 " + nick + " " + 
                                       target->getNicknameDisplay() + " " + 
                                       channel->getNameDisplay() + 
                                       " :They aren't on that channel\r\n");
                    return;
                }
                
                if (sign == '+') {
                    channel->addOperator(target);
                } else {
                    channel->removeOperator(target);
                }
            }
            break;
            
        case 'l':  // user limit
            if (sign == '+') {
                std::string limitStr = getParam(cmd, 2);
                if (limitStr.empty()) {
                    server.sendToClient(fd, Replies::numeric(
                        Replies::ERR_NEEDMOREPARAMS, nick, "MODE",
                        "Not enough parameters"));
                    return;
                }
                int limit = Utils::stringToInt(limitStr);
                if (limit <= 0) {
                    server.sendToClient(fd, Replies::numeric(
                        Replies::ERR_UNKNOWNMODE, nick, "l",
                        "Invalid limit value"));
                    return;
                }
                channel->setUserLimit(limit);
                channel->setMode('l', true);
            } else {
                channel->setUserLimit(0);
                channel->setMode('l', false);
            }
            break;
            
        default:
            // ERR_UNKNOWNMODE (472)
            server.sendToClient(fd, Replies::numeric(
                Replies::ERR_UNKNOWNMODE, nick,
                std::string(1, mode), "is unknown mode char to me"));
            return;
    }
    
    // 7. Broadcast MODE change to all members
    std::string modeMsg = ":" + client.getPrefix() + " MODE " + 
                         channel->getNameDisplay() + " " + modeStr;
    
    // Add parameter for +k, +o, +l, and -o (target nick)
    if ((mode == 'k' || mode == 'l') && sign == '+') {
        modeMsg += " " + getParam(cmd, 2);
    } else if (mode == 'o') {
        modeMsg += " " + getParam(cmd, 2);
    }
    
    modeMsg += "\r\n";
    
    server.sendToClient(fd, modeMsg);
    channel->broadcast(&server, modeMsg, &client);
}
