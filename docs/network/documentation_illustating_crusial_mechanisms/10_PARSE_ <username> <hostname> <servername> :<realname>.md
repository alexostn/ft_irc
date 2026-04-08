***

## `USER` Command — Parameter Anatomy

The `USER` command is part of the **registration handshake** in the IRC protocol (RFC 1459). It is sent by the client **exactly once**, right after `NICK`, 
and completes the registration sequence.

***

### What `<hostname>` and `<servername>` Are

In the original RFC these two fields were designed for **server-to-server** topology:

| Field | RFC intent | Reality |
|---|---|---|
| `<hostname>` | Client's own hostname as it sees itself | Client just sends `0` or `*` — server already knows the real IP |
| `<servername>` | Name of the server being connected to | Ignored by modern servers — client has no knowledge of network topology |

This is exactly why both parameters are **ignored during processing** in our implementation — that is standard modern practice. Clients like Halloy send `0` and `*` there automatically.
***

### Why They Are Still **Stored**

Even though the fields are useless at registration time, they are needed by the **`WHO`** and **`WHOIS`** commands. Both queries return a user profile, 
and the response includes `username`, `hostname`, and `realname`. That is why `Client` exposes a dedicated setter and getters for them
```cpp
// setUsername saves BOTH username and realname in one call
void setUsername(const std::string& username, const std::string& realname);

const std::string& getUsername() const;
const std::string& getHostname() const;  // used by WHO / WHOIS
const std::string& getRealname() const;  // used by WHO / WHOIS
```

The `hostname` from the `USER` message does not need to be stored verbatim — the server can substitute the **real** IP/hostname obtained from `accept()`. 
### How the Example Is Parsed
```
USER  alice      0           *           :Alice Smith
      ^param[0]  ^param[1]   ^param[2]   ^trailing

```

- `param[0]` → `alice` — username, passed to `setUsername()`
- `param[1]` → `0` — hostname, **ignored** (server knows the real address)
- `param[2]` → `*` — servername, **ignored**
- `trailing` → `Alice Smith` — realname, saved via `setUsername(username, realname)` 
***

### What Matters for Your `handleUser`

```cpp
void handleUser(Server& server, Client& client, const Command& cmd) {
    if (client.isRegistered())             // ERR_ALREADYREGISTERED 462
        return;
    if (client.getRegistrationStep() < 2)  // must have PASS + NICK first
        return;                            // ERR_NOTREGISTERED 451

    // param[1] and param[2] are intentionally ignored
    std::string username = cmd.getParam(0);
    std::string realname = cmd.trailing;

    client.setUsername(username, realname);
    // registrationStep -> 3, send RPL_WELCOME 001-004
}
```

According to the team conventions, after a successful `USER` the registration is complete (`registrationStep = 3`), and the server sends the welcome block `001`–`004`. 
The client's `getPrefix()`, which is used in WHO/WHOIS responses, is formatted as `nick!username@hostname` — which is why `hostname` should be populated with the real address 
from `accept()` rather than with the value from the `USER` message.
