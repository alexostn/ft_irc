/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Pong.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sarherna <sarherna@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 14:00:00 by sarherna          #+#    #+#             */
/*   Updated: 2026/02/07 16:00:00 by sarherna         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// PONG command handler
// Client responds to server's PING with PONG
// Format: PONG <server> [<server2>]
// Follows TEAM_CONVENTIONS.md for Halloy compatibility

#include "irc/Server.hpp"
#include "irc/Client.hpp"
#include "irc/Command.hpp"
#include "irc/Replies.hpp"
#include "irc/commands/Pong.hpp"

void handlePong(Server& server, Client& client, const Command& cmd) {
    // Simply acknowledge - client responded to our PING
    // Update last activity timestamp if implementing timeout
    
    // Suppress unused warnings until timeout tracking is implemented
    (void)server;
    (void)client;
    (void)cmd;
    
    // Nothing to do - PONG is just an acknowledgment
}
