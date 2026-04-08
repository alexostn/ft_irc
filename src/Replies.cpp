/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Replies.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akacprzy <akacprzy@student.42warsaw.pl>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/27 00:59:12 by akacprzy          #+#    #+#             */
/*   Updated: 2026/02/08 14:30:16 by akacprzy         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "irc/Replies.hpp"
#include "irc/Client.hpp"
#include "irc/Config.hpp"
#include <sstream>
#include <cstring>

// Numeric reply constants
const std::string Replies::RPL_WELCOME = "001";
const std::string Replies::RPL_YOURHOST = "002";
const std::string Replies::RPL_CREATED = "003";
const std::string Replies::RPL_MYINFO = "004";
const std::string Replies::RPL_CHANNELMODEIS = "324";
const std::string Replies::RPL_NOTOPIC = "331";
const std::string Replies::RPL_TOPIC = "332";
const std::string Replies::RPL_INVITING = "341";
const std::string Replies::RPL_NAMREPLY = "353";
const std::string Replies::RPL_ENDOFNAMES = "366";

// Error reply constants
const std::string Replies::ERR_NOSUCHNICK = "401";
const std::string Replies::ERR_NOSUCHCHANNEL = "403";
const std::string Replies::ERR_CANNOTSENDTOCHAN = "404";
const std::string Replies::ERR_UNKNOWNCOMMAND = "421";
const std::string Replies::ERR_NONICKNAMEGIVEN = "431";
const std::string Replies::ERR_ERRONEUSNICKNAME = "432";
const std::string Replies::ERR_NICKNAMEINUSE = "433";
const std::string Replies::ERR_NICKCOLLISION = "436";
const std::string Replies::ERR_NOTONCHANNEL = "442";
const std::string Replies::ERR_USERONCHANNEL = "443";
const std::string Replies::ERR_NOTREGISTERED = "451";
const std::string Replies::ERR_NEEDMOREPARAMS = "461";
const std::string Replies::ERR_ALREADYREGISTRED = "462";
const std::string Replies::ERR_PASSWDMISMATCH = "464";
const std::string Replies::ERR_CHANNELISFULL = "471";
const std::string Replies::ERR_UNKNOWNMODE = "472";
const std::string Replies::ERR_INVITEONLYCHAN = "473";
const std::string Replies::ERR_BADCHANNELKEY = "475";
const std::string Replies::ERR_BADCHANMASK = "476";
const std::string Replies::ERR_CHANOPRIVSNEEDED = "482";

// Method - numeric()
// Format: ":servername numeric nickname params :trailing\r\n"
// Example: ":irc.example.com 001 user :Welcome to the IRC Network\r\n"
// - Build string with proper formatting
// - Always append \r\n
// - Return formatted string
std::string Replies::numeric(const std::string& numeric, const std::string& nickname, const std::string& params, const std::string& trailing)
{
    std::stringstream ss;
    ss << ":" << Replies::formatServerName() << " " << numeric << " " << nickname;
    if (!params.empty())
        ss << " " << params;
    if (!trailing.empty())
        ss << " :" << trailing;
    ss << "\r\n";
    return ss.str();
}

// Method - command()
// Format: ":prefix command params :trailing\r\n"
// Example: ":nick!user@host PRIVMSG #channel :Hello\r\n"
// - Build string with prefix if provided
// - Always append \r\n
// - Return formatted string
std::string Replies::command(const std::string& prefix, const std::string& cmd, const std::string& params, const std::string& trailing)
{
    std::stringstream ss;
    if (!prefix.empty())
        ss << ":" << prefix << " ";
    ss << cmd;
    if (!params.empty())
        ss << " " << params;
    if (!trailing.empty())
        ss << " :" << trailing;
    ss << "\r\n";
    return ss.str();
}

// Method - simple()
// Format: "command params :trailing\r\n"
// Example: "PING :server.name\r\n"
// - Build string without prefix
// - Always append \r\n
// - Return formatted string
std::string Replies::simple(const std::string& cmd, const std::string& params, const std::string& trailing)
{
    std::stringstream ss;
    ss << cmd;
    if (!params.empty())
        ss << " " << params;
    if (!trailing.empty())
        ss << " :" << trailing;
    ss << "\r\n";
    return ss.str();
}

// Method - formatClientPrefix()
// Format: "nick!user@host"
// - Concatenate nickname, '!', username, '@', hostname
// - Return formatted string
std::string Replies::formatClientPrefix(const Client& client)
{
    return client.getPrefix();
}

// Method - formatServerName()
// - Return server name with leading ':' if needed
// - Or return as-is (depends on usage context)
std::string Replies::formatServerName()
{
    return Config::getServerName();
}
