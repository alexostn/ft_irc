# Team Conventions & Module Contracts

This document defines the interfaces and contracts between modules in the IRC server implementation.

## Team Members

| Role | Name | Responsibilities |
|------|------|------------------|
| **Dev A** | Alex | Network Layer: Server, Poller, Socket I/O, MessageBuffer |
| **Dev B** | Artur | Parser Layer: Parser, Replies, Command Registry |
| **Dev C** | Sara | Logic Layer: Client, Channel, Commands (PASS, NICK, USER, JOIN, etc.) |

---

## 1. Poll() Responsibility

**Responsible:** Alex (Dev A)  
**Coordinates with:** Artur (Dev B) - for event notifications

**Rule: Only the Poller class calls `poll()`**

- The `Poller` class is the **sole** owner of the `poll()` system call
- All I/O multiplexing is centralized in this class
- Other classes must not directly call `poll()`, `select()`, `epoll()`, or `kqueue()`
- The Poller class notifies Server when events occur

**Implementation Contract:**
```cpp
// Only Poller class should have:
int poll(pollfd* fds, nfds_t nfds, int timeout);
```

---

## 2. Sending Responses to Clients

**Responsible:** Alex (Dev A)  
**Coordinates with:** Sara (Dev C) - commands use this to send responses

**Rule: Responses are sent via `Server::sendToClient()`**

- All responses to clients must go through `Server::sendToClient(int clientFd, const std::string& message)`
- Command handlers should NOT directly write to socket file descriptors
- This centralizes I/O operations and allows for proper error handling and logging

**Interface Contract:**
```cpp
class Server {
public:
    // Primary method for sending data to a client
    // Returns: void (logs errors internally, disconnects on fatal error)
    void sendToClient(int clientFd, const std::string& message);
};
```

**Usage Pattern:**
```cpp
// In Command Handler (Sara's code):
server.sendToClient(clientFd, ":server 001 nick :Welcome\r\n");

// NOT:
write(clientFd, "...", ...); // FORBIDDEN
send(clientFd, "...", ...);  // FORBIDDEN
```

---

## 3. Parsed Command Structure Format

**Responsible:** Artur (Dev B)  
**Coordinates with:** Sara (Dev C) - commands receive this structure

**Structure: `Command`**

All parsed IRC messages follow this structure:

```cpp
// include/irc/Command.hpp
struct Command {
    std::string prefix;              // Optional origin (e.g., "nick!user@host")
    std::string command;             // Command name UPPERCASE (e.g., "NICK", "PRIVMSG")
    std::vector<std::string> params; // Command parameters (space-separated)
    std::string trailing;            // Trailing parameter (after ':')
    std::string raw;                 // Original unparsed message
    
    // Helper methods
    bool hasPrefix() const;
    std::string getParam(size_t index) const;  // Returns empty string if index out of bounds
    size_t paramCount() const;
    std::string getFullCommand() const;
};
```

**Examples:**

| Input | prefix | command | params | trailing |
|-------|--------|---------|--------|----------|
| `NICK test\r\n` | "" | "NICK" | ["test"] | "" |
| `PASS secret\r\n` | "" | "PASS" | ["secret"] | "" |
| `USER user 0 * :Real Name\r\n` | "" | "USER" | ["user", "0", "*"] | "Real Name" |
| `PRIVMSG #chan :Hello world\r\n` | "" | "PRIVMSG" | ["#chan"] | "Hello world" |
| `JOIN #test\r\n` | "" | "JOIN" | ["#test"] | "" |
| `JOIN #test key123\r\n` | "" | "JOIN" | ["#test", "key123"] | "" |
| `MODE #test +i\r\n` | "" | "MODE" | ["#test", "+i"] | "" |
| `MODE #test +k password\r\n` | "" | "MODE" | ["#test", "+k", "password"] | "" |
| `KICK #test user :reason\r\n` | "" | "KICK" | ["#test", "user"] | "reason" |

**Parser Contract:**
- Parser must handle incomplete messages (partial data)
- Parser must validate basic IRC message format
- Parser must separate prefix, command, params, and trailing correctly
- **Command is ALWAYS converted to UPPERCASE**
- **Parameters preserve EXACT capitalization from client** (do not modify case)
- **Empty parameters are passed as empty strings, NOT skipped**
- All messages end with `\r\n` (CRLF)
- `getParam(index)` returns empty string if index >= params.size() (never crashes)

**Empty Parameter Handling (IMPORTANT for halloy):**
```cpp
// Input: "USER  0 * :Real Name"  (note: empty username)
// params = ["", "0", "*"]  // Empty string preserved, NOT skipped
// trailing = "Real Name"

// Input: "NICK \r\n"  (NICK with no parameter)
// params = []  // Empty vector (no parameter given)
// This should trigger ERR_NONICKNAMEGIVEN
```

---

## 4. Client Interface

**Responsible:** Sara (Dev C)  
**Coordinates with:** Alex (Dev A) - Server stores/manages Client objects

**Class: `Client`**

```cpp
// include/irc/Client.hpp
class Client {
public:
    // Constructor/Destructor
    Client(int fd);
    ~Client();
    
    // File descriptor
    int getFd() const;
    
    // Identity getters
    const std::string& getNickname() const;        // Returns LOWERCASE (for comparison)
    const std::string& getNicknameDisplay() const; // Returns ORIGINAL CASE (for display)
    const std::string& getUsername() const;
    const std::string& getHostname() const;
    const std::string& getRealname() const;
    
    // Client prefix format: "nick!user@host" (uses display nickname)
    std::string getPrefix() const;
    
    // Registration state
    bool isRegistered() const;
    bool hasPassword() const;
    bool hasNickname() const;
    bool hasUsername() const;
    
    // Registration step tracking (STRICT ORDER: PASS → NICK → USER)
    int getRegistrationStep() const;  // 0=need PASS, 1=need NICK, 2=need USER, 3=registered
    
    // Identity setters (validation done here)
    void setPassword(const std::string& password);
    void setNickname(const std::string& nickname);
    void setUsername(const std::string& username, const std::string& realname);
    void setHostname(const std::string& hostname);
    
    // Password retry mechanism (for halloy compatibility)
    int getPasswordAttempts() const;
    void incrementPasswordAttempts();
    bool hasExceededPasswordAttempts() const;  // Returns true if >= MAX_PASS_ATTEMPTS
    static const int MAX_PASS_ATTEMPTS = 3;
    
    // Registration completion
    bool canRegister() const;  // Returns true if PASS + NICK + USER all set IN ORDER
    void registerClient();     // Mark as fully registered
    
    // Channel membership (tracking only, Channel manages actual membership)
    bool isInChannel(const std::string& channelName) const;
    void addChannel(const std::string& channelName);
    void removeChannel(const std::string& channelName);
    std::vector<std::string> getChannels() const;
    
    // Message buffer management (for receiving data)
    MessageBuffer& getMessageBuffer();
    
private:
    int fd_;
    std::string nickname_;         // Stored LOWERCASE for comparison
    std::string nicknameDisplay_;  // Original case for display (halloy requirement)
    std::string username_;
    std::string hostname_;
    std::string realname_;
    std::string password_;
    
    bool registered_;
    bool passwordSet_;
    bool nicknameSet_;
    bool usernameSet_;
    int registrationStep_;    // 0=PASS, 1=NICK, 2=USER, 3=done
    int passwordAttempts_;    // Counter for failed PASS attempts
    
    std::vector<std::string> channels_;
    MessageBuffer buffer_;
};
```

**Contract:**
- Client manages its own state (registered, nickname, username, etc.)
- Client maintains a buffer for incomplete messages
- Client tracks which channels it belongs to (for cleanup on disconnect)
- Client does NOT directly send data (use `Server::sendToClient()`)
- **Nickname comparison is CASE-INSENSITIVE** (store lowercase for comparison)
- **Nickname display preserves ORIGINAL CASE** (store both versions - halloy requirement)
- **Registration follows STRICT ORDER: PASS → NICK → USER** (halloy requirement)
- **Password retry allowed** (3 attempts before disconnect - halloy requirement)

---

## 5. Channel Interface

**Responsible:** Sara (Dev C)  
**Coordinates with:** Alex (Dev A) - Server stores/manages Channel objects

**Class: `Channel`**

```cpp
// include/irc/Channel.hpp
class Channel {
public:
    // Constructor/Destructor
    Channel(const std::string& name);
    ~Channel();
    
    // Identity
    const std::string& getName() const;
    
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
    
    // Invite list management (for +i mode)
    bool isInvited(const std::string& nickname) const;
    void addToInviteList(const std::string& nickname);
    void removeFromInviteList(const std::string& nickname);
    
    // Topic management
    const std::string& getTopic() const;
    void setTopic(const std::string& topic, const std::string& setBy);
    const std::string& getTopicSetter() const;
    time_t getTopicTime() const;
    bool hasTopic() const;
    
    // Channel modes
    bool hasMode(char mode) const;
    void setMode(char mode, bool value);
    std::string getModeString() const;  // Returns e.g., "+itk"
    
    // Mode-specific getters/setters
    const std::string& getChannelKey() const;   // mode 'k'
    void setChannelKey(const std::string& key);
    int getUserLimit() const;                    // mode 'l'
    void setUserLimit(int limit);
    
    // Broadcasting: send message to all members except exclude
    // NOTE: Message must be fully formatted (with prefix and \r\n)
    void broadcast(Server* server, const std::string& message, Client* exclude = NULL);
    
private:
    std::string name_;
    std::string topic_;
    std::string topicSetter_;
    time_t topicTime_;
    
    std::map<int, Client*> clients_;      // fd -> Client*
    std::vector<Client*> operators_;
    std::set<std::string> inviteList_;    // Nicknames (lowercase)
    
    // Mode flags
    bool inviteOnly_;       // 'i'
    bool topicProtected_;   // 't'
    std::string channelKey_; // 'k' (empty = no key)
    int userLimit_;         // 'l' (0 = no limit)
};
```

**Contract:**
- Channel manages its own member list and operators
- **Channel name comparison is CASE-INSENSITIVE** (store lowercase internally)
- `broadcast()` iterates clients and calls `server->sendToClient()` for each
- `broadcast()` skips `exclude` client (used to not send to sender)
- When a client is removed, also remove from operators and invite list
- Invite list persists even if +i is removed (simplicity)

---

## 6. Server Interface (Methods for Commands)

**Responsible:** Alex (Dev A)  
**Coordinates with:** Sara (Dev C) - commands call these methods

**Methods Sara needs from Server:**

```cpp
class Server {
public:
    // === SENDING ===
    // Primary method for sending data to a client
    void sendToClient(int clientFd, const std::string& message);
    
    // === CLIENT LOOKUP ===
    // Returns NULL if not found (never throws)
    Client* getClient(int fd);
    
    // Returns NULL if not found (case-insensitive search)
    Client* getClientByNickname(const std::string& nickname);
    
    // === CHANNEL MANAGEMENT ===
    // Returns NULL if channel doesn't exist
    Channel* getChannel(const std::string& name);
    
    // Creates and returns new channel (caller must check it doesn't exist first)
    Channel* createChannel(const std::string& name);
    
    // Removes channel from server (call when channel becomes empty)
    void removeChannel(const std::string& name);
    
    // === CLIENT DISCONNECT ===
    // Cleans up client: removes from channels, closes socket, deletes object
    // Does NOT send QUIT message (caller must do that before calling)
    void disconnectClient(int clientFd);
    
    // === CONFIG ===
    const std::string& getPassword() const;
    const std::string& getServerName() const;
    
    // === BRUTE-FORCE PROTECTION (Alex tracks this) ===
    // Note: Password attempt counter is in Client class, but Alex may want
    // to implement additional server-level protection (rate limiting, etc.)
};
```

**Return Value Contract:**
```cpp
// All lookup methods return NULL on not found:
Client* c = server.getClient(fd);
if (!c) {
    // Handle error: client doesn't exist
    return;
}

Client* target = server.getClientByNickname("alice");
if (!target) {
    // SEND ERR_NOSUCHNICK
    return;
}

Channel* chan = server.getChannel("#test");
if (!chan) {
    // Channel doesn't exist - create it or send error depending on command
}
```

---

## 7. Validation Rules

**Responsible:** Sara (Dev C) implements, Artur (Dev B) may use in Parser  
**Coordinates with:** Both need to agree on rules

### 7.1 Nickname Validation

**SIMPLIFIED RULES (not full RFC):**
- Length: 1-9 characters
- First character: letter (a-z, A-Z) or underscore (_)
- Rest: letters, digits (0-9), or underscore (_)
- **Case-insensitive comparison** (store lowercase internally)

```cpp
// Utils::isValidNickname()
bool isValidNickname(const std::string& nickname) {
    if (nickname.empty() || nickname.length() > 9)
        return false;
    
    // First char: letter or underscore
    char first = nickname[0];
    if (!std::isalpha(first) && first != '_')
        return false;
    
    // Rest: letters, digits, underscore
    for (size_t i = 1; i < nickname.length(); ++i) {
        char c = nickname[i];
        if (!std::isalnum(c) && c != '_')
            return false;
    }
    return true;
}

// Examples:
// Valid:   alice, Alice123, _test, user_1
// Invalid: 123user (starts with digit), a-b (hyphen), toolongname (>9)
```

### 7.2 Channel Name Validation

**SIMPLIFIED RULES:**
- Must start with `#` (only # channels, no & local channels)
- Length: 2-50 characters (including #)
- Cannot contain: space, comma, or control characters (0x00-0x1F)
- **Case-insensitive comparison** (store lowercase internally)

```cpp
// Utils::isValidChannelName()
bool isValidChannelName(const std::string& name) {
    if (name.length() < 2 || name.length() > 50)
        return false;
    
    if (name[0] != '#')
        return false;
    
    for (size_t i = 1; i < name.length(); ++i) {
        char c = name[i];
        if (c == ' ' || c == ',' || c < 0x20)
            return false;
    }
    return true;
}

// Examples:
// Valid:   #test, #Test123, #mi-canal
// Invalid: test (no #), # (too short), #test room (space)
```

---

## 8. IRC Numeric Replies Format

**Responsible:** Artur (Dev B) - implements Replies class  
**Coordinates with:** Sara (Dev C) - uses these to send errors

### 8.1 General Format

```
:<servername> <code> <target> [params] :<message>\r\n
```

- `servername`: Server name from config (e.g., "irc.42.fr")
- `code`: 3-digit numeric code
- `target`: Nickname of recipient, or `*` if not yet registered
- `params`: Optional additional parameters
- `message`: Human-readable message after `:`

### 8.2 Error Replies Reference

| Code | Name | Format | When to use |
|------|------|--------|-------------|
| 401 | ERR_NOSUCHNICK | `:srv 401 nick badnick :No such nick/channel` | PRIVMSG/KICK to non-existent user |
| 403 | ERR_NOSUCHCHANNEL | `:srv 403 nick #bad :No such channel` | PART/TOPIC/etc. on non-existent channel |
| 404 | ERR_CANNOTSENDTOCHAN | `:srv 404 nick #chan :Cannot send to channel` | PRIVMSG to channel you're not in |
| 421 | ERR_UNKNOWNCOMMAND | `:srv 421 nick CMD :Unknown command` | Command not recognized |
| 431 | ERR_NONICKNAMEGIVEN | `:srv 431 * :No nickname given` | NICK without parameter |
| 432 | ERR_ERRONEUSNICKNAME | `:srv 432 nick badnick :Erroneous nickname` | NICK with invalid format |
| 433 | ERR_NICKNAMEINUSE | `:srv 433 nick usednick :Nickname is already in use` | NICK already taken |
| 442 | ERR_NOTONCHANNEL | `:srv 442 nick #chan :You're not on that channel` | PART/TOPIC channel you're not in |
| 443 | ERR_USERONCHANNEL | `:srv 443 nick target #chan :is already on channel` | INVITE user already in channel |
| 451 | ERR_NOTREGISTERED | `:srv 451 * :You have not registered` | Command before PASS/NICK/USER |
| 461 | ERR_NEEDMOREPARAMS | `:srv 461 nick CMD :Not enough parameters` | Missing required parameters |
| 462 | ERR_ALREADYREGISTERED | `:srv 462 nick :You may not reregister` | PASS/USER after registration |
| 464 | ERR_PASSWDMISMATCH | `:srv 464 * :Password incorrect` | Wrong password (then disconnect) |
| 471 | ERR_CHANNELISFULL | `:srv 471 nick #chan :Cannot join channel (+l)` | Channel has +l and is full |
| 472 | ERR_UNKNOWNMODE | `:srv 472 nick x :is unknown mode char to me` | Invalid mode character |
| 473 | ERR_INVITEONLYCHAN | `:srv 473 nick #chan :Cannot join channel (+i)` | Channel has +i, not invited |
| 475 | ERR_BADCHANNELKEY | `:srv 475 nick #chan :Cannot join channel (+k)` | Wrong channel key |
| 476 | ERR_BADCHANMASK | `:srv 476 nick #chan :Bad channel mask` | Invalid channel name |
| 482 | ERR_CHANOPRIVSNEEDED | `:srv 482 nick #chan :You're not channel operator` | Need op for KICK/MODE/TOPIC(+t) |

### 8.3 Success Replies Reference

| Code | Name | Format | When to use |
|------|------|--------|-------------|
| 001 | RPL_WELCOME | `:srv 001 nick :Welcome to the IRC Network nick!user@host` | After successful registration |
| 002 | RPL_YOURHOST | `:srv 002 nick :Your host is srv, running version 1.0` | After registration |
| 003 | RPL_CREATED | `:srv 003 nick :This server was created <date>` | After registration |
| 004 | RPL_MYINFO | `:srv 004 nick srv 1.0 o itkol` | After registration (modes supported) |
| 324 | RPL_CHANNELMODEIS | `:srv 324 nick #chan +ik key` | Response to MODE #chan (query) |
| 331 | RPL_NOTOPIC | `:srv 331 nick #chan :No topic is set` | TOPIC query, no topic |
| 332 | RPL_TOPIC | `:srv 332 nick #chan :The topic` | TOPIC query or JOIN with topic |
| 341 | RPL_INVITING | `:srv 341 nick target #chan` | Successful INVITE |
| 353 | RPL_NAMREPLY | `:srv 353 nick = #chan :@op user1 user2` | Names list (@ = op) |
| 366 | RPL_ENDOFNAMES | `:srv 366 nick #chan :End of /NAMES list` | End of names list |

### 8.4 Fatal vs Non-Fatal Errors

| Error | Action |
|-------|--------|
| ERR_PASSWDMISMATCH (464) | Send error, **ALLOW RETRY** (up to 3 attempts, then disconnect) |
| All others | Send error, **DO NOT disconnect** |

**Password Retry Mechanism (halloy compatibility):**
```cpp
// In PASS command handler:
if (password != server.getPassword()) {
    client.incrementPasswordAttempts();
    if (client.hasExceededPasswordAttempts()) {
        // Send error and disconnect
        server.sendToClient(fd, ":srv 464 * :Password incorrect (max attempts exceeded)\r\n");
        server.disconnectClient(fd);
    } else {
        // Send error but allow retry
        server.sendToClient(fd, ":srv 464 * :Password incorrect\r\n");
        // Client can try PASS again
    }
    return;
}
```

---

## 9. Channel Modes (SIMPLIFIED)

**Responsible:** Sara (Dev C)  
**Coordinates with:** Artur (Dev B) - for parsing MODE command

### 9.1 Supported Modes

| Mode | Name | Parameter | Effect |
|------|------|-----------|--------|
| `i` | invite-only | None | Only invited users can JOIN |
| `t` | topic-protected | None | Only operators can change TOPIC |
| `k` | key | string | Requires password to JOIN |
| `o` | operator | nickname | Give/take channel operator status |
| `l` | limit | number | Maximum users in channel |

### 9.2 SIMPLIFIED MODE COMMAND

**DECISION: Only ONE mode change per MODE command**

```
MODE #channel +i
MODE #channel -i
MODE #channel +k password
MODE #channel -k
MODE #channel +o nick
MODE #channel -o nick
MODE #channel +l 10
MODE #channel -l
```

**NOT SUPPORTED (simplification):**
```
MODE #channel +ik password   // Multiple modes - NOT SUPPORTED
MODE #channel +ol nick 10    // Multiple modes - NOT SUPPORTED
```

### 9.3 Mode Parameters in Command struct

| Mode Command | params[0] | params[1] | params[2] |
|--------------|-----------|-----------|-----------|
| `MODE #chan` | "#chan" | - | - |
| `MODE #chan +i` | "#chan" | "+i" | - |
| `MODE #chan +k pass` | "#chan" | "+k" | "pass" |
| `MODE #chan +o nick` | "#chan" | "+o" | "nick" |
| `MODE #chan +l 10` | "#chan" | "+l" | "10" |

### 9.4 Mode Logic

```cpp
// In Mode command handler:
void handleMode(Server& server, Client& client, const Command& cmd) {
    // 1. Check registered
    // 2. Get channel name from params[0]
    // 3. If no params[1]: send current modes (RPL_CHANNELMODEIS)
    // 4. If params[1] exists (e.g., "+i"):
    //    - Check client is operator
    //    - Parse: char sign = params[1][0];  // '+' or '-'
    //    - Parse: char mode = params[1][1];  // 'i', 't', 'k', 'o', 'l'
    //    - Apply mode based on sign
    //    - For +k, +o, +l: get parameter from params[2]
    //    - Broadcast MODE change to channel
}
```

---

## 10. Registration State Machine

**Responsible:** Sara (Dev C)  
**Coordinates with:** Artur (Dev B) - CommandRegistry checks registration

### 10.1 States (STRICT ORDER - halloy requirement)

```
┌─────────────┐
│  STEP 0     │  Waiting for PASS
│  CONNECTED  │  (just connected, nothing set)
└──────┬──────┘
       │ PASS (correct password)
       ▼
┌─────────────┐
│  STEP 1     │  Waiting for NICK
│  HAS_PASS   │  (password accepted)
└──────┬──────┘
       │ NICK (valid & unique)
       ▼
┌─────────────┐
│  STEP 2     │  Waiting for USER
│  HAS_NICK   │  (nickname set)
└──────┬──────┘
       │ USER (valid params)
       ▼
┌─────────────┐
│  STEP 3     │  Can use all commands
│ REGISTERED  │  (send welcome 001-004)
└─────────────┘
```

### 10.2 STRICT Order (halloy requirement)

The order of PASS, NICK, USER is **STRICT**:
- **PASS must come FIRST**
- **NICK must come SECOND** (after PASS)
- **USER must come THIRD** (after NICK)

**If order is violated:**
- NICK before PASS → Send ERR_NOTREGISTERED (451) or silently ignore
- USER before NICK → Send ERR_NOTREGISTERED (451) or silently ignore
- USER before PASS → Send ERR_NOTREGISTERED (451) or silently ignore

### 10.3 Commands Allowed at Each Step

| Step | PASS | NICK | USER | PING/PONG | QUIT | Others |
|------|------|------|------|-----------|------|--------|
| 0 (connected) | **YES** | NO (451) | NO (451) | YES | YES | NO (451) |
| 1 (has pass) | Allow retry | **YES** | NO (451) | YES | YES | NO (451) |
| 2 (has nick) | NO (462) | Allow change | **YES** | YES | YES | NO (451) |
| 3 (registered) | NO (462) | Allow change | NO (462) | YES | YES | **YES** |

### 10.4 Registration Flow in Code

```cpp
// In Client class:
int registrationStep_;  // 0=PASS, 1=NICK, 2=USER, 3=done

// PASS command:
void handlePass(Server& server, Client& client, const Command& cmd) {
    if (client.isRegistered()) {
        // ERR_ALREADYREGISTERED
        return;
    }
    // Step doesn't matter for PASS retry, but must be step 0 or 1
    if (client.getRegistrationStep() > 1) {
        // ERR_ALREADYREGISTERED (already past PASS stage)
        return;
    }
    
    if (cmd.params.empty() || cmd.getParam(0).empty()) {
        // ERR_NEEDMOREPARAMS
        return;
    }
    
    if (cmd.getParam(0) != server.getPassword()) {
        client.incrementPasswordAttempts();
        if (client.hasExceededPasswordAttempts()) {
            // ERR_PASSWDMISMATCH + disconnect
            server.disconnectClient(client.getFd());
        } else {
            // ERR_PASSWDMISMATCH (allow retry)
        }
        return;
    }
    
    client.setPassword(cmd.getParam(0));
    // registrationStep_ becomes 1
}

// NICK command:
void handleNick(Server& server, Client& client, const Command& cmd) {
    if (client.getRegistrationStep() < 1 && !client.isRegistered()) {
        // Must have PASS first
        // ERR_NOTREGISTERED or silently wait
        return;
    }
    // ... validate and set nickname ...
    // If step was 1, move to step 2
}

// USER command:
void handleUser(Server& server, Client& client, const Command& cmd) {
    if (client.isRegistered()) {
        // ERR_ALREADYREGISTERED
        return;
    }
    if (client.getRegistrationStep() < 2) {
        // Must have PASS and NICK first
        // ERR_NOTREGISTERED
        return;
    }
    // ... set username/realname ...
    // Move to step 3, send welcome messages
}
```

### 10.5 Handling Missing/Empty Parameters

| Command | Missing Parameter | Empty Parameter |
|---------|-------------------|-----------------|
| PASS | ERR_NEEDMOREPARAMS (461) | ERR_NEEDMOREPARAMS (461) |
| NICK | ERR_NONICKNAMEGIVEN (431) | ERR_NONICKNAMEGIVEN (431) |
| USER | ERR_NEEDMOREPARAMS (461) | Accept empty username (some clients do this) |

```cpp
// PASS with no parameter:
// Input: "PASS\r\n" or "PASS \r\n"
// Response: ":srv 461 * PASS :Not enough parameters\r\n"

// NICK with no parameter:
// Input: "NICK\r\n" or "NICK \r\n"
// Response: ":srv 431 * :No nickname given\r\n"

// USER with missing parameters:
// Input: "USER\r\n"
// Response: ":srv 461 * USER :Not enough parameters\r\n"
```

---

## 11. Disconnect Behavior

**Responsible:** Alex (Dev A) - implements disconnectClient()  
**Coordinates with:** Sara (Dev C) - QUIT command calls this

### 11.1 Who Does What

| Action | Responsible |
|--------|-------------|
| Build QUIT message | Command handler (Sara) |
| Broadcast QUIT to channels | Command handler (Sara) |
| Remove from all channels | `disconnectClient()` (Alex) |
| Remove from clients map | `disconnectClient()` (Alex) |
| Remove from Poller | `disconnectClient()` (Alex) |
| Close socket | `disconnectClient()` (Alex) |
| Delete Client object | `disconnectClient()` (Alex) |
| Delete empty channels | `disconnectClient()` (Alex) |

### 11.2 QUIT Command Flow

```cpp
// In Quit command handler (Sara):
void handleQuit(Server& server, Client& client, const Command& cmd) {
    // 1. Build quit message
    std::string reason = cmd.trailing.empty() ? "Leaving" : cmd.trailing;
    std::string quitMsg = ":" + client.getPrefix() + " QUIT :" + reason + "\r\n";
    
    // 2. Broadcast to all channels
    std::vector<std::string> channels = client.getChannels();
    for (size_t i = 0; i < channels.size(); ++i) {
        Channel* chan = server.getChannel(channels[i]);
        if (chan) {
            chan->broadcast(&server, quitMsg, &client);  // Don't send to quitting client
        }
    }
    
    // 3. Call disconnect (Alex's code handles cleanup)
    server.disconnectClient(client.getFd());
}
```

### 11.3 Unexpected Disconnect (POLLHUP, recv error)

```cpp
// In Server::handleClientInput() or Poller event handler (Alex):
if (/* POLLHUP or recv returned 0 or error */) {
    // Build quit message
    std::string quitMsg = ":" + client->getPrefix() + " QUIT :Connection closed\r\n";
    
    // Broadcast to channels
    std::vector<std::string> channels = client->getChannels();
    for (size_t i = 0; i < channels.size(); ++i) {
        Channel* chan = getChannel(channels[i]);
        if (chan) {
            chan->broadcast(this, quitMsg, client);
        }
    }
    
    // Cleanup
    disconnectClient(client->getFd());
}
```

---

## 12. Broadcasting Messages

**Responsible:** Sara (Dev C)  
**Coordinates with:** Alex (Dev A) - uses sendToClient()

### 12.1 Single Method: broadcast()

**DECISION: Only use `broadcast()`, no `broadcastFrom()`**

All messages must be fully formatted before calling broadcast:

```cpp
// Channel::broadcast() implementation
void Channel::broadcast(Server* server, const std::string& message, Client* exclude) {
    std::map<int, Client*>::iterator it;
    for (it = clients_.begin(); it != clients_.end(); ++it) {
        if (it->second != exclude) {
            server->sendToClient(it->first, message);
        }
    }
}
```

### 12.2 Usage Examples

```cpp
// JOIN notification
std::string joinMsg = ":" + client->getPrefix() + " JOIN :" + channelName + "\r\n";
channel->broadcast(server, joinMsg, NULL);  // Send to everyone including joiner

// PRIVMSG to channel
std::string privMsg = ":" + client->getPrefix() + " PRIVMSG " + channelName + " :" + text + "\r\n";
channel->broadcast(server, privMsg, client);  // Exclude sender

// PART notification
std::string partMsg = ":" + client->getPrefix() + " PART " + channelName + " :" + reason + "\r\n";
channel->broadcast(server, partMsg, NULL);  // Send to everyone including leaver

// KICK notification
std::string kickMsg = ":" + kicker->getPrefix() + " KICK " + channelName + " " + targetNick + " :" + reason + "\r\n";
channel->broadcast(server, kickMsg, NULL);  // Send to everyone

// MODE change
std::string modeMsg = ":" + client->getPrefix() + " MODE " + channelName + " " + modeStr + "\r\n";
channel->broadcast(server, modeMsg, NULL);  // Send to everyone
```

---

## 13. Case Sensitivity

**Responsible:** All  
**Coordinates with:** All

### 13.1 Rules

| Element | Case Sensitive? | Storage | Comparison | Display |
|---------|-----------------|---------|------------|---------|
| Nickname | NO | Both (see below) | Lowercase | Original |
| Channel name | NO | Both (see below) | Lowercase | Original |
| Command | NO | UPPERCASE | UPPERCASE | N/A |
| Password | YES | As-is | Exact match | N/A |
| Parameters | YES | As-is | As-is | As-is |
| Topic | YES | As-is | N/A | As-is |
| Messages | YES | As-is | N/A | As-is |

### 13.2 Nickname Case Handling (halloy requirement)

**IMPORTANT:** Nicknames must be case-INSENSITIVE for comparison but preserve ORIGINAL case for display.

Example: If user sends `NICK BOB`, then:
- `BOB` and `bob` and `Bob` should all resolve to the SAME user
- When displaying the nickname (in messages, NAMES list, etc.), show `BOB` (original case)

```cpp
// Client stores BOTH versions:
class Client {
private:
    std::string nickname_;         // "bob" - lowercase for comparison
    std::string nicknameDisplay_;  // "BOB" - original for display
    
public:
    // For internal lookups (case-insensitive)
    const std::string& getNickname() const { return nickname_; }
    
    // For display in messages (original case)
    const std::string& getNicknameDisplay() const { return nicknameDisplay_; }
    
    // getPrefix() uses display version
    std::string getPrefix() const {
        return nicknameDisplay_ + "!" + username_ + "@" + hostname_;
    }
    
    void setNickname(const std::string& nickname) {
        nicknameDisplay_ = nickname;           // Preserve original: "BOB"
        nickname_ = Utils::toLower(nickname);  // For comparison: "bob"
        nicknameSet_ = true;
    }
};
```

### 13.3 Server Lookup (case-insensitive)

```cpp
// When searching by nickname (always use lowercase):
Client* Server::getClientByNickname(const std::string& nickname) {
    std::string lower = Utils::toLower(nickname);
    
    std::map<int, Client*>::iterator it;
    for (it = clients_.begin(); it != clients_.end(); ++it) {
        if (it->second->getNickname() == lower) {  // Compare lowercase
            return it->second;
        }
    }
    return NULL;
}

// Example:
// User 1 registered as "BOB"
// User 2 sends: "PRIVMSG bob :Hello"
// Server finds User 1 because "bob" == toLower("BOB")
```

### 13.4 Channel Name Case Handling

Same pattern as nicknames:

```cpp
class Channel {
private:
    std::string name_;         // "#test" - lowercase for comparison
    std::string nameDisplay_;  // "#Test" - original for display
    
public:
    const std::string& getName() const { return name_; }
    const std::string& getNameDisplay() const { return nameDisplay_; }
};
```

### 13.5 Parameter Capitalization (Artur - Parser)

**CRITICAL:** Parser must preserve EXACT capitalization of parameters.

```cpp
// Input: "PRIVMSG BOB :Hello World"
// command = "PRIVMSG"    // UPPERCASE (always)
// params[0] = "BOB"      // PRESERVE original case!
// trailing = "Hello World"  // PRESERVE original case!

// Do NOT convert params to lowercase in parser!
```

---

## 14. Module Interaction Flow

```
┌──────────────────────────────────────────────────────────────────┐
│                           ALEX (Dev A)                           │
│  ┌─────────┐      ┌──────────┐      ┌───────────────┐           │
│  │ Poller  │─────>│  Server  │─────>│ MessageBuffer │           │
│  │ (poll)  │      │ (I/O)    │      │ (CRLF frame)  │           │
│  └─────────┘      └──────────┘      └───────────────┘           │
└───────────────────────────┬──────────────────────────────────────┘
                            │ raw message
                            ▼
┌──────────────────────────────────────────────────────────────────┐
│                          ARTUR (Dev B)                           │
│  ┌──────────┐      ┌─────────────────┐      ┌─────────┐         │
│  │  Parser  │─────>│ CommandRegistry │─────>│ Replies │         │
│  │ (parse)  │      │   (dispatch)    │      │ (format)│         │
│  └──────────┘      └─────────────────┘      └─────────┘         │
└───────────────────────────┬──────────────────────────────────────┘
                            │ Command struct
                            ▼
┌──────────────────────────────────────────────────────────────────┐
│                           SARA (Dev C)                           │
│  ┌──────────────┐      ┌─────────┐      ┌─────────┐             │
│  │   Commands   │─────>│ Client  │      │ Channel │             │
│  │ (handlers)   │      │ (state) │      │ (state) │             │
│  └──────────────┘      └─────────┘      └─────────┘             │
└──────────────────────────────────────────────────────────────────┘
                            │
                            │ formatted response
                            ▼
                    Server::sendToClient()
```

---

## 15. Summary: What Each Developer Needs

### Alex (Dev A) provides to others:

| Method/Class | Used by | Description |
|--------------|---------|-------------|
| `Server::sendToClient()` | Sara | Send formatted message to client |
| `Server::getClient(fd)` | Sara | Get Client* by fd (NULL if not found) |
| `Server::getClientByNickname()` | Sara | Get Client* by nick (NULL if not found) |
| `Server::getChannel()` | Sara | Get Channel* by name (NULL if not found) |
| `Server::createChannel()` | Sara | Create new channel |
| `Server::removeChannel()` | Sara | Remove empty channel |
| `Server::disconnectClient()` | Sara | Clean up and disconnect client |
| `Server::getPassword()` | Sara | Get server password for PASS command |
| `Server::getServerName()` | Sara, Artur | Get server name for replies |

### Artur (Dev B) provides to others:

| Method/Class | Used by | Description |
|--------------|---------|-------------|
| `Command` struct | Sara | Parsed message structure |
| `Parser::parse()` | Alex | Parse raw message into Command |
| `Replies::numeric()` | Sara | Build numeric reply string |
| `Replies::formatPrefix()` | Sara | Build "nick!user@host" prefix |
| `CommandRegistry::execute()` | Alex | Dispatch command to handler |

### Sara (Dev C) provides to others:

| Method/Class | Used by | Description |
|--------------|---------|-------------|
| `Client` class | Alex | Client state management |
| `Channel` class | Alex | Channel state management |
| Command handlers | Artur (registry) | Handle each IRC command |
| `Utils::isValidNickname()` | Artur (optional) | Validate nickname format |
| `Utils::isValidChannelName()` | Artur (optional) | Validate channel name format |

---

## 16. Team Notes

### Initial Architecture Discussion


**Decisions:**
- Use `poll()` (not `epoll` or `kqueue`) for cross-platform compatibility
- Centralize I/O through `Server::sendToClient()`
- Define `Command` structure before starting implementation
- C++98 compliance is mandatory (`-Wall -Wextra -Werror -std=c++98`)
- Makefile must work on both Linux and Mac

### Simplification Decisions

**Decisions:**
- Nickname validation: letters, digits, underscore only (1-9 chars)
- Channel names: only `#` prefix (no `&` local channels)
- MODE command: one mode per command (no `+ik password`)
- Case-insensitive: nicknames and channel names
- broadcast() only (no broadcastFrom())
- QUIT broadcasts, disconnectClient() only cleans up
- Invite list persists even when -i is removed

---

## 17. Halloy Client Compatibility Notes

**Source:** Recommendations from Damian (previous ft_irc team)

Halloy is a common IRC client used for testing. These are specific requirements to ensure compatibility:

### 17.1 Critical Requirements

| Requirement | Responsible | Details |
|-------------|-------------|---------|
| **STRICT registration order** | Sara | PASS → NICK → USER must be in order |
| **Password retry** | Sara + Alex | Allow 3 attempts before disconnect |
| **Preserve nickname case** | Sara | Store original case for display, lowercase for comparison |
| **Preserve parameter case** | Artur | Parser must NOT modify parameter capitalization |
| **Handle empty parameters** | Artur | Pass as empty strings, don't skip them |

### 17.2 Halloy Configuration

Halloy can include nickname, username, realname, and password in config file. It automatically sends these on connection.

**Test scenarios:**
1. Config has password → Client sends PASS automatically
2. Config has wrong password → Client should be able to retry
3. Config has no password → Client may skip PASS (server should reject)
4. Nickname in config → Check case handling (BOB vs bob)

### 17.3 Edge Cases to Test

| Scenario | Expected Behavior |
|----------|-------------------|
| PASS with wrong password | ERR_PASSWDMISMATCH, allow retry |
| PASS retry after 3 failures | Disconnect |
| NICK "BOB" then PRIVMSG to "bob" | Should find same user |
| NICK before PASS | ERR_NOTREGISTERED or ignore |
| USER before NICK | ERR_NOTREGISTERED or ignore |
| Empty NICK parameter | ERR_NONICKNAMEGIVEN |
| Empty PASS parameter | ERR_NEEDMOREPARAMS |
| Multiple spaces in message | Preserve empty params |

### 17.4 Quote from Damian

> "Configuring the client is quite short, but had some tricky elements. In halloy's case it should have the port and address of the server you want to connect to. But it can also include a nickname, username, realname, and password in the config file which the client should automatically forward to the server upon connection. You should also handle cases where there is no nickname, username, realname, or password in the config file, as well as cases where the password was entered incorrectly and the client should retry entering the password. You also should be careful with case sensitivity. Nicknames are not case sensitive in IRC meaning BOB and bob should be recognised as the same user. There is a lot of edge cases and these I've listed are just a few. You'll have to dig through documentation to really do IRC properly."

---

## 18. Summary of Changes for Halloy Compatibility

### Sara (Logic Layer):
- [ ] Implement STRICT order: PASS → NICK → USER
- [ ] Track registration step (0, 1, 2, 3)
- [ ] Store both lowercase and display nickname
- [ ] Implement password retry counter (max 3)
- [ ] Handle empty parameters correctly

### Artur (Parser Layer):
- [ ] Preserve EXACT parameter capitalization
- [ ] Handle empty parameters as empty strings (don't skip)
- [ ] Command to UPPERCASE, params unchanged

### Alex (Network Layer):
- [ ] Don't disconnect immediately on PASS failure
- [ ] Support password retry mechanism
- [ ] (Optional) Rate limiting for brute-force protection
