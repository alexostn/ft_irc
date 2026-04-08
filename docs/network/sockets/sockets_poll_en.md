# Sockets & poll() in ft_irc

## Setup Before First `poll()`

```cpp
socket()   // create server fd → serverSocketFd
bind()     // attach to port
listen()   // tell kernel: queue incoming TCP connections
fcntl(serverSocketFd, F_SETFL, O_NONBLOCK)
poller->addFd(serverSocketFd, POLLIN)
```

`listen()` does **not** wait — it just arms the kernel queue. The actual waiting is `poll()`.

---

## `pollfd` Fields

```c
struct pollfd {
    int   fd;       // which fd to watch
    short events;   // what YOU request  (written before poll)
    short revents;  // what HAPPENED     (written by kernel after poll)
};
```

---

## New Connection Flow

When a client connects, kernel sets `revents = POLLIN` on `serverSocketFd`.  
`accept()` returns a **new separate fd** (the client socket). `serverSocketFd` stays in the array forever.

```
poll() returns → revents & POLLIN on serverSocketFd
  → accept()           → clientFd
  → fcntl(clientFd, F_SETFL, O_NONBLOCK)
  → poller->addFd(clientFd, POLLIN)
```

---

## `pollfd` Array Over Time

| State | fds in array |
|---|---|
| No clients | `serverSocketFd` |
| 1 client | `serverSocketFd`, `clientFd1` |
| 2 clients | `serverSocketFd`, `clientFd1`, `clientFd2` |
| 1 disconnected | `serverSocketFd`, `clientFd2` |

Each `poll()` checks **all** fds in one call. Every `revents` is independent per fd.

---

## Full ASCII Workflow

```
Server::start()
│
├─► socket()           → serverSocketFd created
├─► bind()             → attached to port
├─► listen()           → kernel queues incoming TCP connections
├─► fcntl(serverSocketFd, F_SETFL, O_NONBLOCK)
└─► poller->addFd(serverSocketFd, POLLIN)
         │
         ▼
┌─────────────────────────────┐
│     Server::run()  loop     │◄──────────────────────┐
└─────────────────────────────┘                       │
         │                                            │
         ├─► poller->poll(1000)                       │
         │        │                                   │
         │        ├─ ready == 0 ? (timeout)           │
         │        │     YES ✓ → loop again ───────────┘
         │        │     NO  ✗ ↓                       │
         │                                            │
         └─► poller->processEvents()                  │
                  │                                   │
                  │   [iterate pollfd array]           │
                  │                                   │
                  ├─► fd == serverSocketFd ?           │
                  │     YES ✓ → revents & POLLIN ?     │
                  │               YES ✓ ↓              │
                  │                                   │
                  │       Server::handleNewConnection()│
                  │               │                   │
                  │               ├─► accept(serverSocketFd)
                  │               │        → clientFd (NEW fd)
                  │               ├─► fcntl(clientFd, F_SETFL, O_NONBLOCK)
                  │               ├─► new Client(clientFd)
                  │               ├─► new MessageBuffer(clientFd)
                  │               └─► poller->addFd(clientFd, POLLIN)
                  │                             │     │
                  │                             ▼     └── loop again ──┘
                  │               [clientFd now in pollfd array]
                  │
                  └─► fd == clientFd ?
                        YES ✓ → revents & POLLIN ?
                        │         YES ✓ → Server::handleClientInput(fd)
                        │                       │
                        │                       ├─► recv(fd, buf)
                        │                       │     ├─ bytes > 0  → msgBuffer->append()
                        │                       │     │               extractMessages()
                        │                       │     │               → parse → execute
                        │                       │     ├─ bytes == 0 → disconnectClient(fd)
                        │                       │     └─ EAGAIN      → return (next poll)
                        │
                        └── revents & POLLOUT ?
                                  YES ✓ → Server::flushClientSendQueue(fd)
                                  NO  ✗ → loop again ─────────────────────┘
```

[closing server socket not influence client sockets](close_server_socket_en.md)