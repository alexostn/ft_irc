/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CommandRegistry.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akacprzy <akacprzy@student.42warsaw.pl>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/26 23:31:17 by akacprzy          #+#    #+#             */
/*   Updated: 2026/02/08 13:41:56 by akacprzy         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "irc/CommandRegistry.hpp"
#include "irc/Server.hpp"
#include "irc/Client.hpp"
#include "irc/Command.hpp"
#include "irc/Replies.hpp"
#include "irc/Utils.hpp"
#include "irc/commands/Pass.hpp"
#include "irc/commands/Nick.hpp"
#include "irc/commands/User.hpp"
#include "irc/commands/Join.hpp"
#include "irc/commands/Part.hpp"
#include "irc/commands/Privmsg.hpp"
#include "irc/commands/Invite.hpp"
#include "irc/commands/Kick.hpp"
#include "irc/commands/Topic.hpp"
#include "irc/commands/Mode.hpp"
#include "irc/commands/Quit.hpp"
#include "irc/commands/Ping.hpp"
#include "irc/commands/Pong.hpp"
#include <cctype>

// Constructor
CommandRegistry::CommandRegistry() {
    initializeHandlers();
}

// Destructor
CommandRegistry::~CommandRegistry() {}

// Method - execute()
// - Convert command.command to uppercase
// - Look up handler in handlers_ map
// - If found, call handler(server, client, cmd)
// - If not found, send ERR_UNKNOWNCOMMAND
// - Return true if executed, false if not found
bool CommandRegistry::execute(Server& server, Client& client, const Command& cmd)
{
    if (cmd.command.empty())
        return false;

    std::string upperCmd = Utils::toUpper(cmd.command);

    std::map<std::string, CommandHandler>::iterator it = handlers_.find(upperCmd);
    if (it != handlers_.end())
	{
        it->second(server, client, cmd);
        return true;
    }

    // Error reporting
    std::string nick = client.getNicknameDisplay();
    if (nick.empty())
        nick = "*";
    std::string msg = Replies::numeric(Replies::ERR_UNKNOWNCOMMAND, nick, cmd.command, "Unknown command");
    server.sendToClient(client.getFd(), msg);
	return false;
}

// Method - registerCommand()
// - Convert command to uppercase
// - Store in handlers_ map
void CommandRegistry::registerCommand(const std::string& command, CommandHandler handler)
{
    std::string upperCmd = Utils::toUpper(command);
    handlers_[upperCmd] = handler;
}

// Method - hasCommand()
// - Convert to uppercase
// - Check if exists in handlers_ map
bool CommandRegistry::hasCommand(const std::string& command) const
{
    std::string upperCmd = Utils::toUpper(command);
    return handlers_.find(upperCmd) != handlers_.end();
}

// Method - initializeHandlers()
// - Register all command handlers
void CommandRegistry::initializeHandlers()
{
    registerCommand("PASS", handlePass);
    registerCommand("NICK", handleNick);
    registerCommand("USER", handleUser);
    registerCommand("JOIN", handleJoin);
    registerCommand("PART", handlePart);
    registerCommand("QUIT", handleQuit);
    registerCommand("PRIVMSG", handlePrivmsg);
    registerCommand("INVITE", handleInvite);
    registerCommand("KICK", handleKick);
    registerCommand("TOPIC", handleTopic);
    registerCommand("MODE", handleMode);
    registerCommand("PING", handlePing);
    registerCommand("PONG", handlePong);
}
