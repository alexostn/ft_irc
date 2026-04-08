/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: oostapen <oostapen@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 14:00:00 by sarherna          #+#    #+#             */
/*   Updated: 2026/03/22 00:06:47 by oostapen         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>

// Client class - represents a connected IRC client
// Manages client state, registration, and message buffer
// Follows TEAM_CONVENTIONS.md for Halloy compatibility
class Client {
public:
    // Constructor: initialize with file descriptor
    Client(int fd);
    
    // Destructor
    ~Client();
    
    // File descriptor
    int getFd() const;
    
    // Identity getters
    // getNickname() returns LOWERCASE for comparison
    // getNicknameDisplay() returns ORIGINAL case for display
    const std::string& getNickname() const;
    const std::string& getNicknameDisplay() const;
    const std::string& getUsername() const;
    const std::string& getHostname() const;
    const std::string& getRealname() const;
    
    // Identity setters
    // setNickname stores both lowercase (comparison) and original (display)
    void setNickname(const std::string& nickname);
    void setUsername(const std::string& username, const std::string& realname);
    void setHostname(const std::string& hostname);
    
    // Registration state machine (STRICT ORDER: PASS -> NICK -> USER)
    // Step 0: CONNECTED (waiting for PASS)
    // Step 1: HAS_PASS (waiting for NICK)
    // Step 2: HAS_NICK (waiting for USER)
    // Step 3: REGISTERED (fully registered)
    int getRegistrationStep() const;
    bool isRegistered() const;    // step == 3
    bool hasPassword() const;     // step >= 1
    bool hasNickname() const;     // step >= 2
    
    // State transitions
    void setPassword();           // step 0 -> 1
    void completeNickStep();      // step 1 -> 2
    void registerClient();        // step 2 -> 3
    
    // Password retry mechanism (Halloy: allow up to 3 attempts)
    int getPasswordAttempts() const;
    void incrementPasswordAttempts();
    bool hasExceededPasswordAttempts() const;
    
    // Channel membership
    void addChannel(const std::string& channelName);
    void removeChannel(const std::string& channelName);
    const std::vector<std::string>& getChannels() const;
    bool isInChannel(const std::string& channelName) const;
    
    // Message buffer management (for receiving data)
    void appendToBuffer(const std::string& data);
    std::string& getBuffer();
    void clearBuffer();
    
    // Send queue management (for EAGAIN handling with POLLOUT)
    void               enqueueSend(const std::string& data);
    bool               hasPendingSend() const;
    const std::string& getSendQueue() const;
    void               drainSendQueue(size_t bytesSent);
    
    // Client prefix format: "nick!user@host" (uses display nickname)
    std::string getPrefix() const;
    
private:
    int fd_;
    
    // Identity (case handling for Halloy)
    std::string nickname_;         // lowercase for comparison
    std::string nicknameDisplay_;  // original case for display
    std::string username_;
    std::string hostname_;
    std::string realname_;
    
    // Registration state machine (STRICT ORDER)
    int registrationStep_;         // 0=PASS, 1=NICK, 2=USER, 3=done
    int passwordAttempts_;         // Max 3 attempts
    static const int MAX_PASSWORD_ATTEMPTS = 3;
    
    // Channel membership (stores lowercase channel names)
    std::vector<std::string> channels_;
    
    // Send queue for EAGAIN handling with POLLOUT
    std::string sendQueue_;
    
    // NOTE: NO MessageBuffer here (Variant 3)
};

#endif // CLIENT_HPP
