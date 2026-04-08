/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Quit.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sarherna <sarherna@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 14:00:00 by sarherna          #+#    #+#             */
/*   Updated: 2026/02/07 16:00:00 by sarherna         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// QUIT command handler
// Disconnects client from server
// Format: QUIT [<reason>]
// Follows TEAM_CONVENTIONS.md for Halloy compatibility

#include "irc/Server.hpp"
#include "irc/Client.hpp"
#include "irc/Channel.hpp"
#include "irc/Command.hpp"
#include "irc/Replies.hpp"
#include "irc/commands/Quit.hpp"

void handleQuit(Server& server, Client& client, const Command& cmd) {
    // 1. Build QUIT message
    std::string reason = cmd.trailing.empty() ? "Leaving" : cmd.trailing;
    std::string quitMsg = ":" + client.getPrefix() + " QUIT :" + 
                          reason + "\r\n";
    
    // 2. Broadcast to all channels the client is in
    const std::vector<std::string>& channels = client.getChannels();
    
    for (size_t i = 0; i < channels.size(); ++i) {
        Channel* chan = server.getChannel(channels[i]);
        if (chan) {
            // Broadcast to all members EXCEPT the quitting client
            chan->broadcast(&server, quitMsg, &client);
        }
    }
    
    // 3. Disconnect client (Alex's responsibility)
    // disconnectClient() will:
    // - Remove from all channels
    // - Remove from clients_ map
    // - Close socket
    // - Delete Client object
    server.disconnectClient(client.getFd());
}
