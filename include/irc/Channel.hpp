/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sarherna <sarherna@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 14:00:00 by sarherna          #+#    #+#             */
/*   Updated: 2026/02/07 16:00:00 by sarherna         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <ctime>

// Forward declarations
class Client;
class Server;

// Channel class - represents an IRC channel
// Manages channel members, operators, topic, and modes
// Follows TEAM_CONVENTIONS.md for Halloy compatibility
class Channel {
public:
    // Constructor: create channel with name
    Channel(const std::string& name);
    
    // Destructor
    ~Channel();
    
    // Identity getters (case handling for Halloy)
    const std::string& getName() const;         // lowercase for comparison
    const std::string& getNameDisplay() const;  // original case for display
    
    // Membership management
    bool hasClient(Client* client) const;
    void addClient(Client* client);
    void removeClient(Client* client);
    std::vector<Client*> getClients() const;
    size_t getClientCount() const;
    bool isEmpty() const;
    
    // Operator management
    bool isOperator(Client* client) const;
    void addOperator(Client* client);
    void removeOperator(Client* client);
    std::vector<Client*> getOperators() const;
    
    // Invite list management (case-insensitive)
    bool isInvited(const std::string& nickname) const;
    void addToInviteList(const std::string& nickname);
    void removeFromInviteList(const std::string& nickname);
    
    // Topic management
    const std::string& getTopic() const;
    void setTopic(const std::string& topic, const std::string& setBy);
    const std::string& getTopicSetter() const;
    time_t getTopicTime() const;
    bool hasTopic() const;
    
    // Channel modes (i=invite-only, t=topic-protected, k=key-protected, o=operator, l=user-limit)
    bool hasMode(char mode) const;
    void setMode(char mode, bool value);
    std::string getModeString() const;
    const std::string& getChannelKey() const;
    void setChannelKey(const std::string& key);
    int getUserLimit() const;
    void setUserLimit(int limit);
    
    // Broadcasting: send message to all members except exclude
    // Signature from TEAM_CONVENTIONS.md section 12
    void broadcast(Server* server, const std::string& message, Client* exclude);
    
private:
    // Identity (case handling for Halloy)
    std::string name_;         // "#test" - lowercase for comparison
    std::string nameDisplay_;  // "#Test" - original case for display
    
    // Topic
    std::string topic_;
    std::string topicSetter_;  // nickname who set it
    time_t topicTime_;         // when it was set
    
    // Member storage
    std::map<int, Client*> clients_;    // fd -> Client* for quick lookup
    std::vector<Client*> operators_;    // List of operators
    std::set<std::string> inviteList_;  // lowercase nicknames
    
    // Channel modes
    bool inviteOnly_;      // mode 'i'
    bool topicProtected_;  // mode 't'
    std::string channelKey_;  // mode 'k' (password)
    int userLimit_;        // mode 'l' (0 = no limit)
};

#endif // CHANNEL_HPP
