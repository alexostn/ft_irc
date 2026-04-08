# Closing `serverSocketFd` — Effect on Client Sockets

## What Happens on `close(serverSocketFd)`

| What | Result |
|---|---|
| Existing `clientFd` (5, 6, 7...) | **Alive** — established connections are unaffected |
| New incoming connections | **Impossible** — port stops listening |
| `poll()` on `clientFd` | **Works** — read/write as before |

---

## Why They Are Independent

After `accept()`, the kernel created a full TCP channel bound to the pair  
`(server ip:port, client ip:port)`. That channel does not depend on whether `fd=4` is still listening.

```
close(serverSocketFd=4)
       ↓
  port 6667 stops accepting new SYN packets
       ↓
  BUT: fd=5, fd=6, fd=7 are established TCP sessions in kernel
  they live until explicitly closed or client disconnects
```

---

## Analogy

> Receptionist went home — no new calls accepted.  
> Everyone **already on the line** keeps talking.

---

## Correct Shutdown for ft_irc

All fds must be closed **explicitly**:

```cpp
for (auto& [fd, client] : clients)
    close(fd);           // close each clientFd first
close(serverSocketFd);   // then close server fd
```

Skipping any `clientFd` = fd leak — valgrind will catch it during eval.


