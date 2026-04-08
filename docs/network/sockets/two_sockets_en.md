# Two Sockets After `accept()`

## Analogy: Phone Switchboard

| Role | fd | Analogy |
|---|---|---|
| `serverSocketFd = 4` | listens on port | **Receptionist** — takes calls, never talks itself |
| `clientFd = 5` | talks to one client | **Handset** — dedicated to one caller |

---

## What Happens in the Kernel

```
Before accept():
  fd=4  →  [server socket]  LISTEN
                 │
                 │  clients connect → kernel queues them
                 ▼
           [ queue: conn1, conn2, conn3 ]

After accept():
  fd=4  →  [server socket]  LISTEN       ← unchanged
  fd=5  →  [client socket]  ESTABLISHED  ← new, taken from queue
```

`accept()` pops one connection from the queue and wraps it in a new fd.  
`fd=4` is untouched — keeps listening.

---

## They Are Parallel, Not Nested

```
pollfd array after three accept() calls:
  [ fd=4 (POLLIN) ]   ← permanent, catches new clients
  [ fd=5 (POLLIN) ]   ← client 1
  [ fd=6 (POLLIN) ]   ← client 2
  [ fd=7 (POLLIN) ]   ← client 3
```

| Metaphor | Role |
|---|---|
| `fd=4` | Pipe factory — produces connections |
| `fd=5..N` | The pipes — one per client |
