/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sarherna <sarherna@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/07 00:00:00 by sarherna          #+#    #+#             */
/*   Updated: 2026/02/07 00:00:00 by sarherna          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Client implementation
// Manages client state, registration
// Follows TEAM_CONVENTIONS.md for Halloy compatibility

#include "irc/Client.hpp"
#include "irc/Utils.hpp"
#include <algorithm>
//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>

// Constructor: initialize with file descriptor
Client::Client(int fd)
    : fd_(fd)
    , hostname_("unknown")
    , registrationStep_(0)
    , passwordAttempts_(0)
{
}

// Destructor
Client::~Client() {
    // Cleanup if needed
    // Buffer cleanup handled automatically by std::string destructor
}

// ============================================================================
// File descriptor
// ============================================================================

int Client::getFd() const {
    return fd_;
}

// ============================================================================
// Identity getters
// ============================================================================

// Returns LOWERCASE nickname for comparison
const std::string& Client::getNickname() const {
    return nickname_;
}

// Returns ORIGINAL case nickname for display (Halloy requirement)
const std::string& Client::getNicknameDisplay() const {
    return nicknameDisplay_;
}

const std::string& Client::getUsername() const {
    return username_;
}

const std::string& Client::getHostname() const {
    return hostname_;
}

const std::string& Client::getRealname() const {
    return realname_;
}

// ============================================================================
// Identity setters
// ============================================================================

// Set nickname with case handling (Halloy requirement)
// Stores both lowercase (for comparison) and original (for display)
void Client::setNickname(const std::string& nickname) {
    nicknameDisplay_ = nickname;              // "BOB" - original for display
    nickname_ = Utils::toLower(nickname);     // "bob" - lowercase for comparison
}

void Client::setUsername(const std::string& username, const std::string& realname) {
    username_ = username;
    realname_ = realname;
}

void Client::setHostname(const std::string& hostname) {
    hostname_ = hostname;
}

// ============================================================================
// Registration state machine (STRICT ORDER: PASS -> NICK -> USER)
// ============================================================================

int Client::getRegistrationStep() const {
    return registrationStep_;
}

// Client is fully registered (step 3)
bool Client::isRegistered() const {
    return registrationStep_ >= 3;
}

// Client has provided correct password (step >= 1)
bool Client::hasPassword() const {
    return registrationStep_ >= 1;
}

// Client has set nickname (step >= 2)
bool Client::hasNickname() const {
    return registrationStep_ >= 2;
}

// Transition: step 0 -> 1 (PASS accepted)
void Client::setPassword() {
    if (registrationStep_ == 0) {
        registrationStep_ = 1;
    }
}

// Transition: step 1 -> 2 (NICK set)
void Client::completeNickStep() {
    if (registrationStep_ == 1) {
        registrationStep_ = 2;
    }
}

// Transition: step 2 -> 3 (USER set, fully registered)
void Client::registerClient() {
    if (registrationStep_ == 2) {
        registrationStep_ = 3;
    }
}

// ============================================================================
// Password retry mechanism (Halloy: allow up to 3 attempts)
// ============================================================================

int Client::getPasswordAttempts() const {
    return passwordAttempts_;
}

void Client::incrementPasswordAttempts() {
    ++passwordAttempts_;
}

bool Client::hasExceededPasswordAttempts() const {
    return passwordAttempts_ >= MAX_PASSWORD_ATTEMPTS;
}

// ============================================================================
// Channel membership
// ============================================================================

void Client::addChannel(const std::string& channelName) {
    // Store lowercase for case-insensitive comparison
    std::string lower = Utils::toLower(channelName);
    if (!isInChannel(lower)) {
        channels_.push_back(lower);
    }
}

void Client::removeChannel(const std::string& channelName) {
    std::string lower = Utils::toLower(channelName);
    std::vector<std::string>::iterator it = 
        std::find(channels_.begin(), channels_.end(), lower);
    if (it != channels_.end()) {
        channels_.erase(it);
    }
}

const std::vector<std::string>& Client::getChannels() const {
    return channels_;
}

bool Client::isInChannel(const std::string& channelName) const {
    std::string lower = Utils::toLower(channelName);
    return std::find(channels_.begin(), channels_.end(), lower) != channels_.end();
}

// ============================================================================
// Client prefix format
// ============================================================================

// Format: "nick!user@host" (uses display nickname for Halloy compatibility)
std::string Client::getPrefix() const {
    return nicknameDisplay_ + "!" + username_ + "@" + hostname_;
}

// ============================================================================
// Send queue management (for EAGAIN handling with POLLOUT)
// ============================================================================

void Client::enqueueSend(const std::string& data) {
    sendQueue_ += data;
}

bool Client::hasPendingSend() const {
    return !sendQueue_.empty();
}

const std::string& Client::getSendQueue() const {
    return sendQueue_;
}

void Client::drainSendQueue(size_t bytesSent) {
    if (bytesSent >= sendQueue_.size()) {
        sendQueue_.clear();
    } else {
        sendQueue_.erase(0, bytesSent);
    }
}

