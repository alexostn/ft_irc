
# Networking Terminology & Acronyms

**Part of the networking documentation suite** — [← Back to main pipeline](01_WORKFLOW_PIPELINE.md)

---
FCNTL 
stands for file control — it manipulates properties of an open file descriptor, analogous to ioctl (input-output control) which targets I/O devices. The header <fcntl.h> is officially described as "file control options" in POSIX.

WHY only fcntl(fd, F_SETFL, O_NONBLOCK) is Allowed:

Prevents workarounds like F_SETLKW blocking waits that bypass poll
Enforces the intended architecture: one poll loop + non-blocking fds, nothing else

F_SETFL    = Set File Status Flags
O_NONBLOCK = Open Non-Blocking

NIC — Network Interface Card

The NIC literally asks the CPU for attention.

ISR	Interrupt Service Routine

IRC	Internet Relay Chat
 Relay — from Old French relayer, “to change horses at a station.” 

TCP	Transmission Control Protocol
 Transmission — from Latin transmittere, “to send across.”

IDT	Interrupt Descriptor Table

DMA	Direct Memory Access

APIC	Advanced Programmable Interrupt Controller

RAM	Random Access Memory 

Access — in contrast to a tape, where data is read sequentially. Any cell can be accessed in the same amount of time, regardless of its address.

IRQ — Interrupt Request
Is a physical signal on the wire between the NIC and the CPU. When a packet arrives, the NIC literally raises the voltage on that wire. The CPU detects this at the hardware level—not by checking during a cycle, but by responding to the signal—and interrupts whatever it was doing.

After that, the scheduler checks: are there any processes that were waiting for this specific file descriptor? If so — our ircserv — it changes its state from SLEEPING to RUNNABLE, and on the next CPU time slice, `poll()` from our program returns control.

The name `poll()` is a bit of a paradox: the word means “to poll in a loop,” but the function itself does exactly the opposite—it sleeps and waits. The name dates back to a time when the only alternative was to manually check all file descriptors in a loop.


The complete chain from packet to `poll()`:---

**IRQ (Interrupt Request)** is a physical signal on the wire between the NIC and the CPU. When a packet arrives, the NIC literally raises the voltage on this wire. The CPU detects this at the hardware level—not by checking in a loop, but by responding to the signal—and interrupts whatever it was doing.

Next, the kernel launches the **ISR (Interrupt Service Routine)**—a small handler function that copies data from the NIC buffer to the socket buffer and marks the fd as readable.

After that, the **scheduler** checks: are there any processes waiting specifically for this fd? Yes—your `ircserv`. It changes its state from `SLEEPING` to `RUNNABLE`, and at the next CPU time slice, our `poll()` returns control.

---

### In summary—three levels, not a single loop

| Level | Mechanism | How does it wait? |
|---|---|---|
| Hardware (NIC → CPU) | Hardware interrupt / IRQ | Physical signal on the wire |
| Kernel | Interrupt handler + scheduler | Queues waiting on fd |
| Your code | `poll()` | Thread sleeps until the scheduler wakes it up |

“Someone waving a hand” is literally an electrical signal from the network card. No polling, just a reaction to a physical event. This is precisely why event-based architecture is so efficient: the CPU only runs when there is actual data.





EAGAIN —  POSIX-"Resource unavailable yet, try again" ErrorAGAIN

| Mask    | Description           | Meaning                     |
| ------- | --------------------- | ----------------------------|
| POLLIN  | POLL + IN  (input)    | Data ready to be read       |
| POLLOUT | POLL + OUT (output)   | Can write without blocking  |
| POLLERR | POLL + ERR (error)    | Error on the descriptor     |
| POLLHUP | POLL + HUP (hangup)   | Connection terminated(telef)|
| POLLPRI | POLL + PRI (priority) | Priority/exclusive data 	|


# address family
```
socket(AF_INET, SOCK_STREAM, 0)  // IPv4 — 32-bit address (4 bytes)
socket(AF_INET6, SOCK_STREAM, 0)  // IPv6 — 128-bit address (16 bytes)
socket(AF_UNIX,  SOCK_STREAM, 0)  // not a network at all — local file socket
```

The ft_irc module specifies “TCP/IP (v4 or v6)”, so AF_INET fully meets the requirements.

If you wanted to support both, you would need either 
AF_INET6 with the IPV6_V6ONLY = 0 option, or two separate sockets.
```
IPV6_V6ONLY = 0
```
# function—setsockopt with SO_REUSEADDR
is there for an important reason: without it, after restarting the server, the kernel keeps the port in the TIME_WAIT state for about 2 minutes, 
and bind() fails with “Address already in use.” With this option, you can reuse the address immediately.
________________________________________________________________________
ss -tulnp

`ss` — **socket statistics** (replaces the old `netstat` utility)

Each letter is a separate flag:

| Flag | Full word | Meaning |
|------|-----------|---------|
| `-t` | **TCP** | show TCP sockets |
| `-u` | **UDP** | show UDP sockets |
| `-l` | **listening** | only sockets waiting for connections |
| `-n` | **numeric** | don't resolve names, show raw numbers (IPs and ports) |
| `-p` | **processes** | show which process owns the socket |

Without `-n`:
```
127.0.0.1:postgresql   # resolves 5432 → service name
```

With `-n`:
```
127.0.0.1:5432         # shows the number as-is
```

Without `-l` it would also show established connections — browser, curl, etc. With `-l` — only what is **waiting** for incoming connections, i.e. servers.

##  Related Documentation

| Document | Topic |
|---|---|
| [01_WORKFLOW_PIPELINE.md](01_WORKFLOW_PIPELINE.md) | Complete call chain using these terms and concepts |
| [02_SOCKET_QUEUE_BACKLOG.md](02_SOCKET_QUEUE_BACKLOG.md) | TCP handshake process (SYN, ACK, connection queue) |
| [03_v4_v6_in_socket_VERIFIED_AGAINST_SUBJECT.md](03_v4_v6_in_socket_VERIFIED_AGAINST_SUBJECT.md) | Socket creation, poll() architecture, compliance |
