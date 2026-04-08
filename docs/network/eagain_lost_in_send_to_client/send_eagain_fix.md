# Fix: `sendToClient()` — silent data loss on `EAGAIN`

---

## The problem

```cpp
// Current code
ssize_t sent = send(clientFd, msg.c_str(), msg.size(), 0);
if (sent < 0) {
    if (errno == EPIPE || errno == ECONNRESET)
        disconnectClient(clientFd);
    else if (errno != EAGAIN && errno != EWOULDBLOCK)
        std::cerr << "[Server] send() error..." << std::endl;
    // EAGAIN → falls through silently. Message is gone.
}
```

`EAGAIN` means the kernel's send buffer is full — the OS refused to accept
the data. The message was never sent. Ignoring it means **silent data loss**.

When does this happen in practice:
- Client is paused with `Ctrl+Z` and another client floods the channel
- Very slow network link
- Client's receive window is full (TCP flow control)

---

## Proper fix — per-client send queue with POLLOUT

This is the production-grade solution. Instead of sending immediately,
enqueue the message. Use `POLLOUT` to drain the queue when the socket
is writable again.

### Step 1 — Add a send queue to `Client`

```cpp
// Client.hpp
class Client {
public:
    void               enqueueSend(const std::string& data);
    bool               hasPendingSend() const;
    const std::string& getSendQueue() const;
    void               drainSendQueue(size_t bytesSent);
private:
    std::string sendQueue_;
};

// Client.cpp
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
    sendQueue_.erase(0, bytesSent);
}
```

### Step 2 — `sendToClient()` enqueues, does not block

```cpp
void Server::sendToClient(int clientFd, const std::string& message)
{
    std::string msg = message;
    if (msg.size() < 2 || msg.substr(msg.size() - 2) != "\r\n")
        msg += "\r\n";

    Client* client = getClient(clientFd);
    if (!client) return;

    // If queue already has data, append — preserve order
    if (client->hasPendingSend()) {
        client->enqueueSend(msg);
        return;
    }

    // Try to send immediately
    ssize_t sent = send(clientFd, msg.c_str(), msg.size(), 0);
    if (sent < 0) {
        if (errno == EPIPE || errno == ECONNRESET) {
            disconnectClient(clientFd);
            return;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Buffer full — queue the whole message, register POLLOUT
            client->enqueueSend(msg);
            poller_->setEvents(clientFd, POLLIN | POLLOUT);
            return;
        }
        std::cerr << "[Server] send() error fd=" << clientFd
                  << ": " << strerror(errno) << std::endl;
        return;
    }
    // Partial send — queue the remainder
    if (static_cast<size_t>(sent) < msg.size()) {
        client->enqueueSend(msg.substr(sent));
        poller_->setEvents(clientFd, POLLIN | POLLOUT);
    }
}
```

### Step 3 — drain the queue when `POLLOUT` fires

```cpp
// New method: Server::flushClientSendQueue(int fd)
void Server::flushClientSendQueue(int fd)
{
    Client* client = getClient(fd);
    if (!client || !client->hasPendingSend()) return;

    const std::string& queue = client->getSendQueue();
    ssize_t sent = send(fd, queue.c_str(), queue.size(), 0);
    if (sent > 0) {
        client->drainSendQueue(static_cast<size_t>(sent));
    }
    if (sent < 0 && (errno == EPIPE || errno == ECONNRESET)) {
        disconnectClient(fd);
        return;
    }
    // If queue is now empty, stop watching POLLOUT
    if (!client->hasPendingSend()) {
        poller_->setEvents(fd, POLLIN);
    }
}
```

### Step 4 — call `flushClientSendQueue` from `Poller::processEvents()`

```cpp
// In Poller::processEvents(), client branch:
if (revents & POLLOUT) {
    server_->flushClientSendQueue(fd);
}
if (revents & (POLLIN | POLLHUP)) {
    server_->handleClientInput(fd);
}
```

### Step 5 — add `Poller::setEvents()` helper

```cpp
// Poller.hpp
void setEvents(int fd, short events);

// Poller.cpp
void Poller::setEvents(int fd, short events) {
    int index = findFdIndex(fd);
    if (index != -1)
        pollfds_[index].events = events;
}
```

---
## The Problem — Message Vanishes

The server is a **postman** delivering a letter to a client's mailbox.
The mailbox is **full** — the OS says `EAGAIN`: *"not now, try later"*.

The **old code** read this as an error and **silently threw the letter in the trash**.
The client never received it.

This happened when a client paused (`Ctrl+Z`), the network lagged,
or the server flooded a slow client with messages.

---

## The Fix — Postman's Bag 

Instead of dropping the message, the postman **puts it in a bag**
and waits until the mailbox has space again.

Three new parts:

- **`sendQueue_` (the bag)** — a string inside each Client storing messages
  that couldn't be sent yet.
- **`POLLOUT` (the doorbell)** — server asks the OS: *"ring me when the socket
  is writable again"*. No busy-waiting, just a notification.
- **`flushClientSendQueue()` (emptying the bag)** — when `POLLOUT` fires,
  drain the queue and send everything stored.

---

## Step by Step

1. Server calls `sendToClient()`.
2. Tries `send()` — if OK, done ✅
3. `EAGAIN` → enqueue message, add `POLLOUT` to poll events.
4. `poll` detects socket is writable → fires `POLLOUT`.
5. `flushClientSendQueue()` sends queued data.
6. Queue empty → remove `POLLOUT`, back to `POLLIN` only.

---

## Before / After

| Situation          | Old code              | New code                     |
|--------------------|-----------------------|------------------------------|
| `EAGAIN` on send   |  Message lost       |  Queued, sent later         |
| Partial send       |  Remainder lost     |  Remainder queued           |
| Slow / paused client |  Data dropped     |  Wait for `POLLOUT`, retry  |

**Result:** zero silent data loss — every message is either delivered or
the client is disconnected.


