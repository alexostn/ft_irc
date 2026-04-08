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
                  │   [iterate pollfd array]          │
                  │                                   │
                  ├─► fd == serverSocketFd ?          │
                  │     YES ✓ → revents & POLLIN ?    │
                  │               YES ✓ ↓             │
                  │                                   │
                  │      Server::handleNewConnection()│
                  │               │                   │
                  │               ├─► accept(serverSocketFd)
                  │               │        → clientFd (NEW fd)
                  │               ├─► fcntl(clientFd, F_SETFL, O_NONBLOCK)
                  │               ├─► new Client(clientFd)
                  │               ├─► new MessageBuffer(clientFd)
                  │               └─► poller->addFd(clientFd, POLLIN)
                  │                        │          │
                  │                        │          └── loop again ──┘
                  │                        ▼
                  │               [clientFd now in pollfd array]
                  │
                  └─► fd == clientFd ?
                        YES ✓ → revents & POLLIN ?
                        │         YES ✓ → Server::handleClientInput(fd)
                        │                       │
                        │                       ├─► recv(fd, buf)
                        │                       │     ├─ bytes > 0  → msgBuffer->append()
                        │                       │     │               msgBuffer->extractMessages()
                        │                       │     │               → parse → execute
                        │                       │     ├─ bytes == 0 → disconnectClient(fd)
                        │                       │     └─ EAGAIN      → return (wait next poll)
                        │                       └─► ...
                        │
                        └── revents & POLLOUT ?
                                  YES ✓ → Server::flushClientSendQueue(fd)
                                  NO  ✗ → loop again ─────────────────────┘
```

| Iteration           | fd in the array                       |
| ------------------ | -------------------------------------- |
| Before clients        | serverSocketFd                      |
| After the 1st client | serverSocketFd, clientFd1            |
| After the 2nd client | serverSocketFd, clientFd1, clientFd2 |
| Client disconnected | serverSocketFd, clientFd2             |