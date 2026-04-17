# Stateless Behavior in the IRC Server (・・ ) ... 
## Overview

This IRC server is **stateless** — it does not save information about users, messages, or channels to disk or any persistent storage. When a client disconnects, all data about that client and their messages is *immediately erased*. If you restart the server, all users must reconnect and reconfigure everything.

---

## What is "Stateless"? (Non-Technical Explanation)

Think of a stateless server like a **temporary chat room in a parking lot**:

- **When you arrive:** You introduce yourself ("My name is Bob, I'm here for the tech talk")
- **While you're there:** You chat with others, and they hear you
- **When you leave:** Everything about you vanishes. Your name, your conversations — gone.
- **When you return the next day:** The server doesn't remember you. You have to introduce yourself all over again.
- **Nobody's memories persist:** Even if you had a long conversation with John about the weather, nobody wrote it down. John doesn't see your messages after you leave, and you can't ask "what did I talk about yesterday?"

In a **stateful** system (like Facebook), your profile, messages, and friends list are saved in a database. You log in and all your history is there.

This IRC server is the **parking lot model** — everything is temporary.

---

## Specific Examples from the Code

### 1. **Client Objects are Immediately Deleted on Disconnect**

**File:** [src/Server.cpp](src/Server.cpp#L349-L380)  
**Function:** `Server::disconnectClient(int fd)`

When a client disconnects, the entire Client object is deleted:

```cpp
void Server::disconnectClient(int fd) {
    // ... broadcast QUIT message ...
    
    // Remove from channels
    clients_.erase(fd);              // ← Client deleted from memory
    
    // Delete MessageBuffer
    delete buffer;                   // ← Received messages deleted
    
    // Remove from Poller
    poller_->removeFd(fd);
    
    // Close socket
    close(fd);
}
```

**What happens to user data:**
- **Nickname:** Lost (example: "alice" exists only in `client->nickname_`)
- **Username/Realname:** Lost (example: "alice1" / "Alice Smith" are deleted)
- **Hostname:** Lost (example: "user@192.168.1.1" is forgotten)
- **Channels joined:** Completely erased from `client->channels_` vector

---

### 2. **Message History is Never Saved to Disk**

**File:** [include/irc/MessageBuffer.hpp](include/irc/MessageBuffer.hpp)  
**File:** [src/MessageBuffer.cpp](src/MessageBuffer.cpp)

The MessageBuffer is a temporary holder for incomplete messages:

```cpp
class MessageBuffer {
    private:
        std::string buffer_;    // ← Only RAM, never written to disk
        
    public:
        void append(const std::string& data);
        std::vector<std::string> extractMessages();  // ← Parsed, then discarded
        void clear();           // ← Messages are deleted
};
```

**What happens:**
1. Client sends: `PRIVMSG #chat :Hello everyone`
2. Server receives bytes → temporarily stored in `buffer_`
3. Parser extracts the complete message
4. Command executes (broadcasts to channel)
5. The message is **never logged anywhere**
6. When client closes the tab, they can't see old messages
7. If server crashes, all in-flight messages are lost

---

### 3. **Channel History and Topic Disappear When Channel Empties**

**File:** [include/irc/Channel.hpp](include/irc/Channel.hpp#L28-L45)  
**File:** [src/Channel.cpp](src/Channel.cpp)

```cpp
bool Channel::isEmpty() const {
    return clients_.empty();
}

void Server::disconnectClient(int fd) {
    // ... remove client from channel ...
    
    if (chan->isEmpty()) {
        removeChannel(channels[i]);  // ← Entire channel deleted!
    }
}
```

**What happens to channel data:**
- **Topic:** Lost (`topic_` is deleted)
- **Invite list:** Lost (`inviteList_` is deleted)
- **Modes:** Lost (`inviteOnly_`, `topicProtected_`, `channelKey_`, `userLimit_` are deleted)
- **Member list:** Lost (all `operators_` deleted)

**Concrete example:**
```
Day 1, 2:00 PM
- User alice joins #general
- User bob joins #general  
- alice sets topic: "Weekly standup notes"
- alice posts: "John fixed the deploy bug"
- bob posts: "Great work!"
- bob leaves, alice leaves
- ✓ #general is now empty → entire channel deleted from memory

Day 1, 2:15 PM
- alice rejoins #general
- alice sees: Empty channel, no topic, no history
- She doesn't see "John fixed the deploy bug" or her previous message
```

---

### 4. **User Profiles Don't Persist Between Connections**

**File:** [include/irc/Client.hpp](include/irc/Client.hpp)

```cpp
class Client {
    private:
        std::string nickname_;        // ← Only in RAM
        std::string username_;        // ← Only in RAM
        std::string hostname_;        // ← Only in RAM
        std::string realname_;        // ← Only in RAM
};
```

**Example scenario:**
```
Connection 1 (Morning)
- alice connects from 192.168.1.100
- Sets nick: "alice", username: "alice1", realname: "Alice Smith"
- Sets hostname: "user@192.168.1.100"
- Chats for 30 minutes
- Leaves (connection drops)
→ All 4 pieces of identity deleted

Connection 2 (Afternoon)
- Same alice connects from 192.168.1.101  
- Server doesn't recognize her (no database)
- She has to set nick, username, etc. again from scratch
- Hostname is now "user@192.168.1.101"
```

---

### 5. **Parser is Temporary — Messages Don't Get Logged**

**File:** [include/irc/Parser.hpp](include/irc/Parser.hpp)  
**File:** [src/Parser.cpp](src/Parser.cpp#L1-L60)

```cpp
bool Parser::parse(const std::string& message, Command& cmd) {
    cmd.raw = message;        // ← Only temporary
    cmd.command = "";
    cmd.params.clear();
    cmd.trailing = "";
    
    // ... parse the message ...
    
    return true;        // No storage, message is not saved
}
```

**What's missing:**
- No timestamp recorded
- No sender username saved
- No recipient list saved
- No forwarding to offline users
- No audit log

---

### 6. **Channel Broadcasts are Immediate, Not Queued**

**File:** [src/Channel.cpp](src/Channel.cpp#L130-L150)  
**Function:** `Channel::broadcast(...)`

```cpp
void Channel::broadcast(Server* server, const std::string& message, Client* exclude) {
    std::vector<Client*> clients = getClients();
    
    for (size_t i = 0; i < clients.size(); ++i) {
        if (clients[i] != exclude) {
            server->sendToClient(clients[i]->getFd(), message);  // ← Sent NOW
        }
    }
    // Message is not stored in a list, just sent once and forgotten
}
```

**Example:**
```
Scenario: alice sends "Hello" to #chat

Timeline:
1. alice sends: PRIVMSG #chat :Hello
2. Server broadcasts to bob (online)
3. Server sends to carol (online)
4. (nobody offline, so no queue)
5. Message is gone forever

If david was offline:
- He doesn't get the message when he connects later
- He misses "Hello"
```

---

### 7. **Client Registration State Resets After Disconnect**

**File:** [include/irc/Client.hpp](include/irc/Client.hpp#L31-L46)  
**File:** [src/Client.cpp](src/Client.cpp#L80-L120)

```cpp
class Client {
    private:
        int registrationStep_;      // ← Step 0,1,2,3 — lost on disconnect
        int passwordAttempts_;      // ← Counter lost on disconnect
};
```

**Register sequence (must happen in order):**
```
Step 0: Client connects
Step 1: Sends PASS <password> → if correct, step becomes 1
Step 2: Sends NICK <nickname>  → step becomes 2  
Step 3: Sends USER ...          → step becomes 3 (fully registered)

After disconnection:
- If alice disconnects at step 2 (has password + nick, waiting for USER)
- When alice reconnects, server treats her as step 0 again
- She must start over: PASS, NICK, USER
```

---

### 8. **No Persistent Authentication**

**File:** [include/irc/Config.hpp](include/irc/Config.hpp)

```cpp
class Config {
    private:
        std::string password_;      // ← Only in memory during runtime
};

std::string Server::getPassword() const {
    return config_.getPassword();   // ← Checked from RAM, not a database
}
```

**What doesn't happen:**
- No user accounts database
- No password hashing or salting
- No user roles/permissions saved
- After server restart, the same password still works (it's hardcoded at startup), but all users are forgotten

---

### 9. **No Message Queue for Offline Users**

**File:** [src/commands/Privmsg.cpp](src/commands/Privmsg.cpp)

```cpp
void handlePrivmsg(Server& server, Client& client, const Command& cmd) {
    // ... check if target exists ...
    
    Client* targetClient = server.getClientByNickname(target);
    
    if (!targetClient) {
        // User is offline
        server.sendToClient(fd, Replies::numeric(
            Replies::ERR_NOSUCHNICK, nick, target,
            "No such nick/channel"));  // ← Message is rejected, not queued
        return;
    }
    
    // Send directly to online user
    server.sendToClient(targetClient->getFd(), message);
    // No record is kept if user goes offline immediately after
}
```

**Example:**
```
3:00 PM
- alice sends to bob: "Can you review my PR?"
- bob receives it immediately
- bob's client window is open

3:05 PM
- bob closes his laptop and disconnects
- bob's received messages are gone from server

3:10 PM
- alice sends to bob: "Bob, did you see my message?"
- bob is offline
- Message returns: "No such nick/channel" error
- bob never sees either message when he reconnects later
```

---

### 10. **Server Restart = Complete Data Loss**

**File:** [src/main.cpp](src/main.cpp)

```cpp
int main(int argc, char** argv) {
    try {
        Config config = Config::parseArgs(argc, argv);
        Server server(config);           // ← New instance, empty maps
        server.start();
        server.run();
    }
    // ← When run() exits, destructor deletes all clients, channels, buffers
}
```

**Scenario:**
```
Server running for 2 hours
- 100 users connected
- 50 channels created
- Millions of messages exchanged
- Channel #announcements has important topic set

Server administrator does: "kill -9 <pid>"
- Destructor called
- ALL Client objects deleted (100 users' data gone)
- ALL Channel objects deleted (50 channels gone)
- ALL MessageBuffer objects deleted (recent messages gone)

Server restarts
- Users see: "Connection refused"
- They reconnect, introduce themselves again
- Nobody remembers what happened before the crash
- Nobody can see the old #announcements topic
```

---

## Summary: What IS NOT Persistent

| Data | Status | Example |
|------|--------|---------|
| User nickname | Lost on disconnect | alice's "alice" nickname   |
| User profile | Lost on disconnect | alice1 / Alice Smith / user@192.168.1.100 |
| Password records | Lost on restart | Server accepts same password, but users forgotten |
| Channel membership | Lost on disconnect | alice's membership in #tech, #gaming |
| Channel topic | Lost when empty | "#general: No topic" → old topic lost |
| Channel modes | Lost when empty | Invite-only flag, key, limits deleted |
| Message history | Never saved | alice's "hello world" message |
| Private message queue | Never queued | Messages to offline users are rejected |
| Invite lists | Lost when empty | alice was invited to #vip, nobody remembers |
| Password attempts | Lost on disconnect | alice's retry counter resets each connection |
| Registration state | Lost on disconnect | alice starts over at PASS step, not mid-registration |

## Summary: What IS Preserved (During the Session)

| Data | Status | Example |
|------|--------|---------|
| Connected clients | Preserved (in RAM) | alice is online now |
| Active channels | Preserved (in RAM) | #chat exists because it has members |
| Messages in-transit | Preserved briefly | Received data in MessageBuffer until parsed |
| Current topic | Preserved (in RAM) | #chat topic visible to all members |
| Socket connection | Preserved | alice's fd stays open until disconnect |

---

## Why Stateless?

This is **by design** for an IRC server project as part of the 42 school curriculum. The focus is on:

1. **Network programming** — Managing sockets, I/O, concurrency
2. **Protocol parsing** — Understanding IRC message format
3. **Real-time communication** — Broadcasting, event loops
4. **No database complexity** — Keeps the project scope reasonable

A production IRC server (like UnrealIRCD or ngIRCd) *would* add:
- Persistent user database (SQLite, PostgreSQL)
- Message logging
- Channel persistence
- Authentication systems
- User profiles with avatars, settings, etc.

But that's beyond the scope of this educational project.

---

## Files with Stateless Behavior

| File | Function | Stateless Behavior |
|------|----------|-------------------|
| [src/Server.cpp](src/Server.cpp) | `disconnectClient()` | Deletes all client data |
| [src/Server.cpp](src/Server.cpp) | `~Server()` | Destructor cleans all clients, channels, buffers |
| [include/irc/Client.hpp](include/irc/Client.hpp) | Constructor & fields | Client data only exists in RAM |
| [src/Client.cpp](src/Client.cpp) | `~Client()` | No persistent cleanup (data was in-memory only) |
| [include/irc/Channel.hpp](include/irc/Channel.hpp) | `isEmpty()` / `removeClient()` | Channel deleted when empty |
| [src/Channel.cpp](src/Channel.cpp) | Constructor & broadcast | Channel data only in RAM |
| [src/MessageBuffer.cpp](src/MessageBuffer.cpp) | `extractMessages()` | Messages parsed then discarded |
| [src/Parser.cpp](src/Parser.cpp) | `parse()` | Messages parsed but never logged |
| [src/commands/Privmsg.cpp](src/commands/Privmsg.cpp) | `handlePrivmsg()` | Message rejected if user offline, not queued |
| [src/main.cpp](src/main.cpp) | `main()` | Server restart loses all data |

---

## Testing This Behavior

If you want to verify the stateless nature:

### Test 1: Client Disconnect
```bash
# Terminal 1: Run server
./ircserv 6667 password

# Terminal 2: Connect with nc
nc localhost 6667
> PASS password
> NICK alice
> USER alice 0 * :Alice Smith
# (chat for a bit, set a topic in a channel)
# Press Ctrl+D to disconnect

# Terminal 3: Reconnect as same user
nc localhost 6667
> PASS password
> NICK alice
> (no memory of previous session)
```

### Test 2: Server Restart
```bash
# (with a connected client and active channels)
# Kill server: Ctrl+C
# Restart server: ./ircserv 6667 password

# All clients see "Connection lost"
# All channels are gone (test by rejoining — no topic)
```

### Test 3: Message History
```bash
# Client 1 and 2 in #chat
# Client 1: PRIVMSG #chat :This is a message
# Client 1 disconnects
# Client 1 reconnects
# Result: No message history visible
```

