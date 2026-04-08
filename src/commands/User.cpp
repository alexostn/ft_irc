/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   User.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sarherna <sarherna@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 14:00:00 by sarherna          #+#    #+#             */
/*   Updated: 2026/02/07 16:00:00 by sarherna         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// USER command handler
// Sets username and realname for client
// Format: USER <username> <mode> <unused> <realname>
// Follows TEAM_CONVENTIONS.md for Halloy compatibility

#include "irc/Server.hpp"
#include "irc/Client.hpp"
#include "irc/Command.hpp"
#include "irc/Replies.hpp"
#include "irc/commands/User.hpp"

// Helper to get param safely (returns empty string if out of bounds)
static std::string getParam(const Command& cmd, size_t index) {
    if (index < cmd.params.size()) {
        return cmd.params[index];
    }
    return "";
}

// Forward declaration of welcome message helper
static void sendWelcomeMessages(Server& server, Client& client);

void handleUser(Server& server, Client& client, const Command& cmd) {
    int fd = client.getFd();
    
    // Get current nick for error messages (use "*" if not set)
    std::string nick = client.getNicknameDisplay();
    if (nick.empty()) {
        nick = "*";
    }
    
    // 1. Already registered?
    if (client.isRegistered()) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_ALREADYREGISTRED,
            nick,
            "",
            "You may not reregister"));
        return;
    }
    
    // 2. Must have PASS and NICK first (step >= 2)
    // STRICT ORDER: PASS -> NICK -> USER
    if (client.getRegistrationStep() < 2) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NOTREGISTERED,
            nick,
            "",
            "You have not registered"));
        return;
    }
    
    // 3. Need at least username (params[0])
    // Format: USER <username> <mode> <unused> <realname>
    if (cmd.params.empty()) {
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NEEDMOREPARAMS,
            nick,
            "USER",
            "Not enough parameters"));
        return;
    }
    
    // 4. Extract username and realname
    // Username is always params[0]
    std::string username = getParam(cmd, 0);
    
    // Realname can be in trailing (after ':') or params[3]
    std::string realname;
    if (!cmd.trailing.empty()) {
        realname = cmd.trailing;
    } else {
        realname = getParam(cmd, 3);
    }
    
    // If no realname provided, use username as default
    if (realname.empty()) {
        realname = username;
    }
    
    // 5. Set user info
    client.setUsername(username, realname);
    
    // 6. Complete registration (step 2 -> 3)
    client.registerClient();
    
    // 7. Send welcome messages (001-004)
    sendWelcomeMessages(server, client);
}

// Send the standard IRC welcome messages after successful registration
static void sendWelcomeMessages(Server& server, Client& client) {
    int fd = client.getFd();
    std::string nick = client.getNicknameDisplay();
    std::string serverName = Replies::formatServerName();
    
    // RPL_WELCOME (001) - Welcome to the network
    server.sendToClient(fd, Replies::numeric(
        Replies::RPL_WELCOME,
        nick,
        "",
        "Welcome to the IRC Network " + client.getPrefix()));
    
    // RPL_YOURHOST (002) - Your host info
    server.sendToClient(fd, Replies::numeric(
        Replies::RPL_YOURHOST,
        nick,
        "",
        "Your host is " + serverName + ", running version 1.0"));
    
    // RPL_CREATED (003) - Server creation date
    server.sendToClient(fd, Replies::numeric(
        Replies::RPL_CREATED,
        nick,
        "",
        "This server was created today"));
    
    // RPL_MYINFO (004) - Server info and supported modes
    // Format: <servername> <version> <available user modes> <available channel modes>
    server.sendToClient(fd, Replies::numeric(
        Replies::RPL_MYINFO,
        nick,
        serverName + " 1.0 o itkol",
        ""));
}
