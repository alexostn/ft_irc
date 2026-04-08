/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sarherna <sarherna@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 14:00:00 by sarherna          #+#    #+#             */
/*   Updated: 2026/02/07 16:00:00 by sarherna         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Channel implementation
// Manages channel members, operators, topic, and modes
// Follows TEAM_CONVENTIONS.md for Halloy compatibility

#include "irc/Channel.hpp"
#include "irc/Client.hpp"
#include "irc/Server.hpp"
#include "irc/Utils.hpp"
#include <algorithm>
#include <ctime>

// Constructor: initialize with channel name
Channel::Channel(const std::string& name)
    : name_(Utils::toLower(name))
    , nameDisplay_(name)
    , topicTime_(0)
    , inviteOnly_(false)
    , topicProtected_(false)
    , userLimit_(0)
{
}

// Destructor
Channel::~Channel() {
    // Cleanup - clients are not owned by Channel, just references
    // No need to delete Client pointers
}

// ============================================================================
// Identity getters (case handling for Halloy)
// ============================================================================

const std::string& Channel::getName() const {
    return name_;  // lowercase for comparison
}

const std::string& Channel::getNameDisplay() const {
    return nameDisplay_;  // original case for display
}

// ============================================================================
// Membership management
// ============================================================================

bool Channel::hasClient(Client* client) const {
    return clients_.find(client->getFd()) != clients_.end();
}

void Channel::addClient(Client* client) {
    clients_[client->getFd()] = client;
}

void Channel::removeClient(Client* client) {
    int fd = client->getFd();
    clients_.erase(fd);
    
    // Also remove from operators if present
    std::vector<Client*>::iterator it = 
        std::find(operators_.begin(), operators_.end(), client);
    if (it != operators_.end()) {
        operators_.erase(it);
    }
}

std::vector<Client*> Channel::getClients() const {
    std::vector<Client*> result;
    for (std::map<int, Client*>::const_iterator it = clients_.begin();
         it != clients_.end(); ++it) {
        result.push_back(it->second);
    }
    return result;
}

size_t Channel::getClientCount() const {
    return clients_.size();
}

bool Channel::isEmpty() const {
    return clients_.empty();
}

// ============================================================================
// Operator management
// ============================================================================

bool Channel::isOperator(Client* client) const {
    return std::find(operators_.begin(), operators_.end(), client) 
           != operators_.end();
}

void Channel::addOperator(Client* client) {
    if (!isOperator(client)) {
        operators_.push_back(client);
    }
}

void Channel::removeOperator(Client* client) {
    std::vector<Client*>::iterator it = 
        std::find(operators_.begin(), operators_.end(), client);
    if (it != operators_.end()) {
        operators_.erase(it);
    }
}

std::vector<Client*> Channel::getOperators() const {
    return operators_;
}

// ============================================================================
// Invite list management (case-insensitive)
// ============================================================================

bool Channel::isInvited(const std::string& nickname) const {
    std::string lower = Utils::toLower(nickname);
    return inviteList_.find(lower) != inviteList_.end();
}

void Channel::addToInviteList(const std::string& nickname) {
    std::string lower = Utils::toLower(nickname);
    inviteList_.insert(lower);
}

void Channel::removeFromInviteList(const std::string& nickname) {
    std::string lower = Utils::toLower(nickname);
    inviteList_.erase(lower);
}

// ============================================================================
// Topic management
// ============================================================================

const std::string& Channel::getTopic() const {
    return topic_;
}

void Channel::setTopic(const std::string& topic, const std::string& setBy) {
    topic_ = topic;
    topicSetter_ = setBy;
    topicTime_ = Utils::getCurrentTimestamp();
}

const std::string& Channel::getTopicSetter() const {
    return topicSetter_;
}

time_t Channel::getTopicTime() const {
    return topicTime_;
}

bool Channel::hasTopic() const {
    return !topic_.empty();
}

// ============================================================================
// Channel modes
// ============================================================================

bool Channel::hasMode(char mode) const {
    switch (mode) {
        case 'i': return inviteOnly_;
        case 't': return topicProtected_;
        case 'k': return !channelKey_.empty();
        case 'l': return userLimit_ > 0;
        default: return false;
    }
}

void Channel::setMode(char mode, bool value) {
    switch (mode) {
        case 'i': inviteOnly_ = value; break;
        case 't': topicProtected_ = value; break;
        case 'k': if (!value) channelKey_.clear(); break;
        case 'l': if (!value) userLimit_ = 0; break;
        default: break;
    }
}

std::string Channel::getModeString() const {
    std::string modes = "+";
    
    if (inviteOnly_) modes += "i";
    if (topicProtected_) modes += "t";
    if (!channelKey_.empty()) modes += "k";
    if (userLimit_ > 0) modes += "l";
    
    // If no modes are set, return empty string
    if (modes == "+") return "";
    
    return modes;
}

const std::string& Channel::getChannelKey() const {
    return channelKey_;
}

void Channel::setChannelKey(const std::string& key) {
    channelKey_ = key;
}

int Channel::getUserLimit() const {
    return userLimit_;
}

void Channel::setUserLimit(int limit) {
    userLimit_ = limit;
}

// ============================================================================
// Broadcasting
// ============================================================================

// Broadcast message to all members except exclude
// Signature from TEAM_CONVENTIONS.md section 12
void Channel::broadcast(Server* server, const std::string& message, 
                        Client* exclude) {
    for (std::map<int, Client*>::iterator it = clients_.begin();
         it != clients_.end(); ++it) {
        if (it->second != exclude) {
            server->sendToClient(it->first, message);
        }
    }
}
