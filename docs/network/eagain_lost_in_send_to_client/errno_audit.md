# errno Usage Audit — ft_irc

> Eval requirement: *"errno must NOT be used to trigger specific actions
> such as reading again after errno == EAGAIN"*
>
> Forbidden pattern: using errno to **retry** I/O without a new `poll()` call.
> Allowed: using errno to **exit silently**, **log**, or **disconnect**.

---

## Audit results

| Location | Call | errno value | Used to | Rule violation | Note |
|---|---|---|---|---|---|
| `Poller::poll()` | `poll()` | `EINTR` | Return 0, re-enter loop | ✅ No | Signal interrupted syscall — standard handling |
| `Server::setNonBlocking()` | `fcntl()` | any | Log error only | ✅ No | No action taken on errno value |
| `Server::handleNewConnection()` | `accept()` | `EAGAIN/EWOULDBLOCK` | Silent return | ✅ No | No retry, waits for next poll() |
| `Server::handleClientInput()` | `recv()` | `EAGAIN/EWOULDBLOCK` | Silent return | ✅ No | No retry, waits for next poll() |
| `Server::sendToClient()` | `send()` | `EPIPE/ECONNRESET` | Disconnect client | ✅ No | Correct broken-pipe handling |
| `Server::sendToClient()` | `send()` | `EAGAIN/EWOULDBLOCK` | Silently ignored | ✅ No (rule) ⚠️ Yes (correctness) | Data is lost with no log — see below |

---

## Only open issue — `sendToClient()` silent data loss

When `send()` returns `-1` with `EAGAIN`, the kernel send buffer is full.
The current code ignores this silently — **the message is dropped with no
error, no log, no retry**.

This does not violate the eval rule (no re-read without poll), but it is a
correctness bug: data sent to a slow or paused client disappears silently.

See `docs/send_eagain_fix.md` for the full fix with a per-client send queue.

---

*All other errno usages in Server.cpp and Poller.cpp are compliant.*
