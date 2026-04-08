# IRC Fundamentals: A Conceptual Guide for Developers

*Understanding the Architecture Before Writing the Code*

---

## Preface

Before you write a single line of code, you must understand what you are building. Too many students rush to implementation without grasping the underlying concepts, leading to confusion and architectural mistakes that are painful to fix later.

This guide will teach you the **what** and **why** of IRC servers. The **how** comes naturally once you understand these foundations.

Think of this document as the blueprint of a building. An architect doesn't start laying bricks; they first understand the purpose of each room, how people will flow through the space, and what structural elements are necessary. You are the architect of your IRC server.

---

## Part I: The Data Model

### Chapter 1: Understanding the Client

When a user connects to your IRC server, they are represented by a **Client** object. But what exactly is a client? Let us examine each component.

#### 1.1 The File Descriptor: Your Only Link to the Outside World

When someone connects to your server, the operating system creates a TCP socket and assigns it a unique integer called a **file descriptor** (fd). This number is your only way to communicate with that specific connection.

Consider this analogy: imagine a restaurant where each table has a number. When the kitchen prepares a dish, the waiter needs that table number to know where to deliver it. Similarly, when your server prepares a message, it needs the file descriptor to know which socket to write to.

```cpp
// In your Client class:
int fd_;  // This is how you identify and communicate with this client
```

**Key insight:** The file descriptor is assigned by the operating system, not by your code. When you call `accept()` on a listening socket, the OS returns a new file descriptor for that connection.

#### 1.2 The Nickname: Public Identity

The nickname is what other users see when someone speaks. It is the public face of a connection.

**Validation Rules (Simplified):**
- Length: 1-9 characters
- Allowed characters: letters (a-z, A-Z), digits (0-9), and underscore (_)
- First character: must be a letter or underscore (never a digit)
- Uniqueness: no two clients can have the same nickname

**Critical Implementation Detail - Case Insensitivity:**

IRC specifies that nicknames are case-insensitive. This means `BOB`, `Bob`, and `bob` all refer to the same user. However, we must preserve the original case for display purposes.

The solution is to store two versions:

```cpp
class Client {
private:
    std::string nickname_;        // "bob" - lowercase, used for comparison
    std::string nicknameDisplay_; // "BOB" - original case, used for display
};
```

When comparing nicknames, always use the lowercase version. When displaying nicknames in messages, use the display version.

#### 1.3 Username and Hostname: Technical Identity

Beyond the nickname, each client has:

- **Username:** A technical identifier provided by the client during registration
- **Hostname:** The IP address or resolved hostname of the client's connection
- **Realname:** An optional descriptive field (often the user's actual name)

Together with the nickname, these form the **client prefix**: `nick!user@host`

For example: `alice!~alice@192.168.1.100`

This prefix appears in every message the client sends, allowing recipients to know who sent it.

#### 1.4 Registration State: The Gateway

A freshly connected client is not yet a full participant in the IRC network. They must first complete the **registration process**.

**The registration follows a STRICT ORDER:**

1. **PASS** - Provide the server password
2. **NICK** - Choose a nickname
3. **USER** - Provide username and realname

This order is not arbitrary. It is a requirement for compatibility with clients like halloy. If a client sends NICK before PASS, or USER before NICK, the server should reject the command.

```cpp
// Registration step tracking
int registrationStep_;  // 0=waiting for PASS, 1=waiting for NICK, 
                        // 2=waiting for USER, 3=fully registered
```

---

### Chapter 2: Understanding Channels

A channel is a named group where multiple clients can communicate simultaneously. Think of it as a chat room.

#### 2.1 Channel Names

Channel names follow specific rules:

- Must begin with `#` (we do not support `&` local channels for simplicity)
- Length: 2-50 characters including the `#`
- Cannot contain: spaces, commas, or control characters
- **Case-insensitive:** `#Test` and `#test` are the same channel

Like nicknames, store both a lowercase version for comparison and the original for display.

#### 2.2 Channel Membership

A channel maintains a list of its members. When a message is sent to the channel, it must be delivered to all members.

**Data structure choice:**

```cpp
std::map<int, Client*> clients_;  // fd -> Client pointer
```

Using the file descriptor as the key allows quick lookup when you need to check if a specific client is in the channel.

#### 2.3 Channel Operators

Some members have elevated privileges. These are **channel operators** (chanops).

**How someone becomes an operator:**
1. The first client to JOIN a new channel automatically becomes its operator
2. An existing operator can grant operator status to another member using `MODE #channel +o nickname`

**What operators can do:**
- KICK: Remove users from the channel
- MODE: Change channel settings
- INVITE: Invite users to invite-only channels
- TOPIC: Change the channel topic (when +t mode is set)

#### 2.4 Channel Modes

Modes are configuration flags that modify channel behavior:

| Mode | Name | Effect |
|------|------|--------|
| `+i` | Invite-only | Only invited users may join |
| `+t` | Topic-protected | Only operators may change the topic |
| `+k` | Key-protected | A password is required to join |
| `+l` | User limit | Maximum number of members |
| `+o` | Operator | Grants operator privileges to a user |

**Simplification:** We process only ONE mode per MODE command. Commands like `MODE #channel +itk password` with multiple modes are not supported.

---

## Part II: The Registration Flow

### Chapter 3: The Authentication Sequence

When a client connects, they cannot immediately participate in IRC activities. They must first prove their identity through registration.

#### 3.1 Step 0: Connection Established

The client has just connected. They have a file descriptor, but nothing else.

**Allowed commands at this step:**
- PASS (to provide the server password)
- PING/PONG (connection keep-alive)
- QUIT (to disconnect)

NICK and USER are NOT allowed until PASS succeeds.

#### 3.2 Step 1: PASS Command

The client sends: `PASS <password>`

**Your server must:**
1. Compare the provided password with the server's configured password
2. If correct: advance the client to step 1 (`registrationStep_ = 1`)
3. If incorrect: send `ERR_PASSWDMISMATCH` (464)

**Password Retry Mechanism:**

Unlike many IRC implementations that disconnect immediately on wrong password, we allow retries. This is important for compatibility with clients like halloy that may need to correct a misconfigured password.

```cpp
static const int MAX_PASS_ATTEMPTS = 3;
int passwordAttempts_;  // Track failed attempts

// In PASS handler:
if (password != serverPassword) {
    passwordAttempts_++;
    if (passwordAttempts_ >= MAX_PASS_ATTEMPTS) {
        // Send error and disconnect
        disconnectClient(fd);
    } else {
        // Send error but allow retry
        sendError(ERR_PASSWDMISMATCH);
    }
    return;
}
```

#### 3.3 Step 2: NICK Command

The client sends: `NICK <nickname>`

**Your server must:**
1. Verify that PASS has been completed (step >= 1)
2. Validate the nickname format
3. Check that the nickname is not already in use (case-insensitive!)
4. Store both lowercase and display versions
5. Advance to step 2

**Case-insensitive uniqueness check:**

```cpp
// Convert to lowercase for comparison
std::string lowerNick = Utils::toLower(nickname);

// Check all existing clients
for (/* each client */) {
    if (client->getNickname() == lowerNick) {  // getNickname() returns lowercase
        // ERR_NICKNAMEINUSE
        return;
    }
}

// Store both versions
client->setNickname(nickname);  // This stores both lowercase and original
```

#### 3.4 Step 3: USER Command

The client sends: `USER <username> <mode> <unused> :<realname>`

**Your server must:**
1. Verify that NICK has been completed (step >= 2)
2. Extract username (from params[0]) and realname (from trailing)
3. Store these values
4. Complete registration and send welcome messages

#### 3.5 Registration Complete

When all three steps are complete, the client is **registered**. Send the welcome messages:

- **001 RPL_WELCOME:** Welcome to the network
- **002 RPL_YOURHOST:** Server information
- **003 RPL_CREATED:** Server creation date
- **004 RPL_MYINFO:** Server capabilities

These messages are expected by IRC clients. Without them, the client may not function correctly.

---

### Chapter 4: The State Machine Visualization

Think of registration as a finite state machine:

```
┌─────────────────────────────────────────────────────┐
│  STATE 0: CONNECTED                                  │
│  Waiting for: PASS                                   │
│  Accepts: PASS, PING, PONG, QUIT                    │
│  Rejects: NICK, USER, JOIN, PRIVMSG, etc.           │
└────────────────────┬────────────────────────────────┘
                     │ PASS correct
                     ▼
┌─────────────────────────────────────────────────────┐
│  STATE 1: HAS_PASSWORD                               │
│  Waiting for: NICK                                   │
│  Accepts: NICK, PASS (retry), PING, PONG, QUIT      │
│  Rejects: USER, JOIN, PRIVMSG, etc.                 │
└────────────────────┬────────────────────────────────┘
                     │ NICK valid and unique
                     ▼
┌─────────────────────────────────────────────────────┐
│  STATE 2: HAS_NICKNAME                               │
│  Waiting for: USER                                   │
│  Accepts: USER, NICK (change), PING, PONG, QUIT     │
│  Rejects: PASS, JOIN, PRIVMSG, etc.                 │
└────────────────────┬────────────────────────────────┘
                     │ USER valid
                     ▼
┌─────────────────────────────────────────────────────┐
│  STATE 3: REGISTERED                                 │
│  Sends: Welcome messages (001-004)                   │
│  Accepts: All commands                               │
└─────────────────────────────────────────────────────┘
```

Each state has clear entry conditions and allowed commands. This makes your implementation clean and debuggable.

---

## Part III: Channel Operations

### Chapter 5: Joining Channels

The JOIN command is how clients enter channels.

#### 5.1 The JOIN Process

When a client sends `JOIN #channel`:

1. **Verify registration:** Is the client fully registered? If not, send `ERR_NOTREGISTERED` (451).

2. **Validate channel name:** Does it start with `#`? Is the format valid?

3. **Check channel modes:**
   - If `+k` (key): Does the client provide the correct key?
   - If `+i` (invite-only): Is the client on the invite list?
   - If `+l` (limit): Is there room?

4. **Get or create the channel:**
   ```cpp
   Channel* channel = getChannel(channelName);  // Case-insensitive lookup
   if (!channel) {
       channel = createChannel(channelName);
   }
   ```

5. **Add the client:**
   ```cpp
   channel->addClient(client);
   if (channel->getClientCount() == 1) {
       channel->addOperator(client);  // First member becomes operator
   }
   ```

6. **Send notifications:**
   - To the joining client: JOIN confirmation, topic (if any), names list
   - To existing members: JOIN notification

#### 5.2 Automatic Channel Creation

There is no explicit "create channel" command in IRC. Channels are created implicitly when the first user joins them. The first joiner automatically becomes the channel operator.

This is elegant design: it reduces complexity while providing intuitive behavior.

---

### Chapter 6: Message Broadcasting

When a client sends a message to a channel, that message must reach all other members.

#### 6.1 The Broadcast Pattern

```cpp
void Channel::broadcast(Server* server, const std::string& message, 
                        Client* exclude) {
    for (std::map<int, Client*>::iterator it = clients_.begin();
         it != clients_.end(); ++it) {
        if (it->second != exclude) {
            server->sendToClient(it->first, message);
        }
    }
}
```

**Why exclude the sender?** Because the sender already knows what they said. Sending their own message back would be redundant (though some implementations do this).

#### 6.2 Message Format

All messages sent to channels include the sender's prefix:

```
:alice!~alice@192.168.1.100 PRIVMSG #general :Hello everyone!
```

This allows recipients to know who sent the message.

---

### Chapter 7: Leaving Channels

Clients can leave channels through PART (voluntary) or be removed through KICK (forced) or QUIT (disconnection).

#### 7.1 PART: Voluntary Departure

```
PART #channel :Goodbye message
```

Process:
1. Verify the channel exists
2. Verify the client is in the channel
3. Broadcast the PART to all members
4. Remove the client from the channel
5. If the channel is now empty, delete it

#### 7.2 QUIT: Disconnection

When a client disconnects:
1. Get all channels the client is in
2. For each channel, broadcast the QUIT message
3. Remove the client from all channels
4. Clean up empty channels
5. Close the socket and delete the Client object

---

## Part IV: Operator Commands

### Chapter 8: The Permission System

Channel operators have elevated privileges. Understanding when to check for operator status is crucial.

#### 8.1 Commands Requiring Operator Status

| Command | Requires Operator? | Additional Condition |
|---------|-------------------|---------------------|
| KICK | Always | Must be in channel |
| MODE (change) | Always | Must be in channel |
| TOPIC (change) | Only if +t mode | Must be in channel |
| INVITE | Only if +i mode | Must be in channel |

#### 8.2 The KICK Command

KICK removes a user from a channel against their will.

```
KICK #channel targetuser :Reason for kick
```

Verification steps:
1. Is the kicker registered?
2. Does the channel exist?
3. Is the kicker in the channel?
4. Is the kicker an operator?
5. Does the target exist?
6. Is the target in the channel?

Only after all verifications pass do you execute the kick.

#### 8.3 The MODE Command

MODE changes channel settings.

**Viewing modes:**
```
MODE #channel
```
Response: Current mode string (e.g., `+itk secretkey`)

**Changing modes (one mode at a time):**
```
MODE #channel +i
MODE #channel +k secretpassword
MODE #channel +o someuser
MODE #channel +l 50
```

Parse the mode character and its optional parameter, verify operator status, apply the change, and broadcast to the channel.

---

## Part V: Data Structures in C++98

### Chapter 9: Choosing the Right Container

C++98 provides several standard containers. Understanding their characteristics helps you choose wisely.

#### 9.1 std::map - For Key-Value Lookups

A map provides O(log n) lookup by key. Use it when you need to find things quickly.

**Use cases in IRC:**
- `std::map<int, Client*>` - Find client by file descriptor
- `std::map<std::string, Channel*>` - Find channel by name

```cpp
Client* getClient(int fd) {
    std::map<int, Client*>::iterator it = clients_.find(fd);
    return (it != clients_.end()) ? it->second : NULL;
}
```

#### 9.2 std::vector - For Ordered Lists

A vector is a dynamic array. Use it for lists you'll iterate over.

**Use cases in IRC:**
- `std::vector<Client*>` - List of operators in a channel

#### 9.3 std::set - For Unique Collections

A set automatically prevents duplicates. Use it when uniqueness matters.

**Use cases in IRC:**
- `std::set<std::string>` - Invite list (lowercase nicknames)

#### 9.4 Memory Ownership

Establish clear ownership rules:

- **Server owns Clients:** `new Client()` in accept, `delete client` on disconnect
- **Server owns Channels:** `new Channel()` on first join, `delete channel` when empty
- **Channels do NOT own Clients:** They only store pointers

Never delete something you don't own.

---

### Chapter 10: Iterators in C++98

Without `auto`, you must write iterator types explicitly.

```cpp
// Iterating over a map
for (std::map<int, Client*>::iterator it = clients_.begin();
     it != clients_.end(); ++it) {
    int fd = it->first;
    Client* client = it->second;
    // ... use fd and client ...
}

// Finding in a map
std::map<int, Client*>::iterator it = clients_.find(fd);
if (it != clients_.end()) {
    Client* found = it->second;
}
```

**Caution with erase:** After calling `erase()` on an iterator, that iterator is invalidated. If iterating while erasing, use the pattern:

```cpp
for (it = map.begin(); it != map.end(); /* no increment here */) {
    if (shouldRemove(it->second)) {
        map.erase(it++);  // Post-increment returns old iterator, then advances
    } else {
        ++it;
    }
}
```

---

## Part VI: Message Routing

### Chapter 11: PRIVMSG and NOTICE

These commands route messages to users or channels.

#### 11.1 Determining the Target Type

The target's first character tells you what it is:

```cpp
if (target[0] == '#') {
    // Channel message
    Channel* channel = getChannel(target);  // Case-insensitive
    // ...
} else {
    // Private message to user
    Client* targetClient = getClientByNickname(target);  // Case-insensitive
    // ...
}
```

#### 11.2 Channel Messages

1. Verify channel exists → `ERR_NOSUCHCHANNEL` (403)
2. Verify sender is in channel → `ERR_CANNOTSENDTOCHAN` (404)
3. Format message with sender prefix
4. Broadcast to channel (excluding sender)

#### 11.3 Private Messages

1. Verify target user exists → `ERR_NOSUCHNICK` (401)
2. Format message with sender prefix
3. Send only to target

#### 11.4 NOTICE vs PRIVMSG

The only difference: NOTICE never generates error responses. If the target doesn't exist, silently do nothing.

This prevents message loops with automated bots.

---

## Part VII: Summary and Checklist

### Chapter 12: Pre-Implementation Checklist

Before writing code, verify you understand:

**Core Concepts:**
- [ ] What is a Client and what properties does it have?
- [ ] What is a Channel and what properties does it have?
- [ ] How does the registration state machine work?
- [ ] What is the difference between an operator and a regular user?

**Protocol Requirements:**
- [ ] Registration order: PASS → NICK → USER (strict)
- [ ] Password retry: 3 attempts before disconnect
- [ ] Nicknames: case-insensitive comparison, preserve display case
- [ ] Channels: only `#` prefix, case-insensitive
- [ ] MODE: one mode per command

**Technical Implementation:**
- [ ] Which data structures for which purpose?
- [ ] Who owns which objects?
- [ ] How to broadcast messages to channels?
- [ ] How to handle errors?

---

### Chapter 13: Implementation Order

Follow this sequence for the smoothest development experience:

1. **Client class**
   - Properties: fd, nickname (both versions), username, hostname, realname
   - Registration tracking: registrationStep_, passwordAttempts_
   - Channel membership tracking

2. **Channel class**
   - Properties: name (both versions), topic, modes
   - Membership: clients map, operators list, invite list
   - Broadcast functionality

3. **Registration commands**
   - PASS: password verification with retry
   - NICK: validation and uniqueness (case-insensitive)
   - USER: completion and welcome messages

4. **Channel commands**
   - JOIN: create/join with mode checks
   - PART: leave with notification
   - QUIT: disconnect from all channels

5. **Operator commands**
   - PRIVMSG: routing to users and channels
   - KICK: removal with permission check
   - INVITE: adding to invite list
   - TOPIC: viewing and changing
   - MODE: one mode at a time

---

## Appendix A: Error Codes Reference

| Code | Name | When Sent |
|------|------|-----------|
| 401 | ERR_NOSUCHNICK | Target user doesn't exist |
| 403 | ERR_NOSUCHCHANNEL | Target channel doesn't exist |
| 404 | ERR_CANNOTSENDTOCHAN | Not in channel |
| 431 | ERR_NONICKNAMEGIVEN | NICK without parameter |
| 432 | ERR_ERRONEUSNICKNAME | Invalid nickname format |
| 433 | ERR_NICKNAMEINUSE | Nickname already taken |
| 442 | ERR_NOTONCHANNEL | Not in the specified channel |
| 451 | ERR_NOTREGISTERED | Command before registration |
| 461 | ERR_NEEDMOREPARAMS | Missing required parameters |
| 462 | ERR_ALREADYREGISTERED | Already registered |
| 464 | ERR_PASSWDMISMATCH | Wrong password |
| 471 | ERR_CHANNELISFULL | Channel +l limit reached |
| 473 | ERR_INVITEONLYCHAN | Channel +i, not invited |
| 475 | ERR_BADCHANNELKEY | Wrong channel key |
| 482 | ERR_CHANOPRIVSNEEDED | Operator privilege required |

---

## Appendix B: Halloy Compatibility Notes

The halloy IRC client has specific requirements that differ from some traditional IRC implementations:

1. **Strict registration order:** PASS must come before NICK, NICK before USER
2. **Password retry:** Allow multiple attempts before disconnecting
3. **Case handling:** Preserve original case for display while comparing case-insensitively
4. **Empty parameters:** Handle empty strings correctly (don't skip them)

These requirements are incorporated throughout this guide.

---

## Closing Thoughts

Building an IRC server is an exercise in protocol implementation, state management, and network programming. The concepts are not individually complex, but their interaction creates a sophisticated system.

Take time to understand each concept before coding. Draw diagrams. Trace through scenarios manually. The implementation will flow naturally from understanding.

Good luck with your ft_irc project.

---

*For technical contracts with team members, see `docs/TEAM_CONVENTIONS.md`*
