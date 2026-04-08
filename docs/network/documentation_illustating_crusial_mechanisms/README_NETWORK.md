# Network Mechanisms & Architecture Documentation

Comprehensive documentation of crucial IRC server mechanisms and internals.

---

## Contents

### [01_WORKFLOW_PIPELINE.md](01_WORKFLOW_PIPELINE.md)
Complete ASCII workflow diagram showing the entire call chain and program flow from `main()` to shutdown. Includes:
- Server initialization & socket setup
- Main event loop architecture
- Connection handling
- Message reception pipeline
- Command parsing & dispatch
- Registration flow (PASS → NICK → USER)
- Command handlers (PASS, NICK, USER, JOIN, PRIVMSG)
- Broadcasting mechanism
- Cleanup & resource deallocation

### [02_SOCKET_QUEUE_BACKLOG.md](02_SOCKET_QUEUE_BACKLOG.md)
Explanation of the `backlog` parameter in `listen()`:
- What backlog means
- Connection acceptance flow
- Queue behavior
- Recommended values for different scenarios

# Network Mechanisms & Architecture Documentation

Comprehensive documentation of crucial IRC server mechanisms and internals.

---

##  Document Structure (Interconnected)

This suite of documents is **cross-linked** to allow deep dives into specific mechanisms while maintaining a coherent overview.

### Master Document
**-> [01_WORKFLOW_PIPELINE.md](01_WORKFLOW_PIPELINE.md)**  
Complete ASCII workflow diagram showing the entire call chain and program flow from `main()` to shutdown.

**Includes inline cross-links to:**
- Socket creation (AF_INET, socket flags)
- Backlog handling in listen()
- poll() mechanism and revents checking
- Non-blocking I/O setup
- Client address extraction

**Contents:**
- Server initialization & socket setup
- Main event loop architecture  
- Connection handling
- Message reception pipeline
- Command parsing & dispatch
- Registration flow (PASS → NICK → USER)
- Command handlers (PASS, NICK, USER, JOIN, PRIVMSG)
- Broadcasting mechanism
- Cleanup & resource deallocation

---

### Deep-Dive Documents

#### [02_SOCKET_QUEUE_BACKLOG.md](02_SOCKET_QUEUE_BACKLOG.md)
Detailed explanation of the `backlog` parameter in `listen()`:
- TCP 3-way handshake mechanics
- Connection queue states
- Queue overflow behavior (RST)
- Impact on high-traffic scenarios
- Your implementation (backlog=10) explained

#### [03_v4_v6_in_socket_VERIFIED_AGAINST_SUBJECT.md](03_v4_v6_in_socket_VERIFIED_AGAINST_SUBJECT.md)
Subject-compliant networking internals guide:
- IPv4 vs IPv6 socket design
- poll() architecture and event flags (POLLIN, POLLOUT, POLLERR, POLLHUP)
- Hardware interrupt chain (IRQ → DMA → ISR → kernel scheduler)
- Non-blocking I/O requirements
- inet_ntoa() vs inet_ntop() (deprecated function notes)
- **Subject compliance checklist** for critical requirements

#### [0_TERMS_MEANING.md](0_TERMS_MEANING.md)
Networking terminology and acronyms explained:
- IRQ, IDT, ISR, DMA, APIC, PIC, RAM, PC
- poll() flag meanings (POLLIN, POLLOUT, POLLERR, POLLHUP, POLLPRI)
- Address families (AF_INET, AF_INET6, AF_UNIX)
- SO_REUSEADDR purpose

---

##  Navigation Pattern

```
01_WORKFLOW_PIPELINE.md (START HERE)
    ├─ Links to: 02_SOCKET_QUEUE_BACKLOG.md
    ├─ Links to: 03_v4_v6_in_socket_VERIFIED_AGAINST_SUBJECT.md
    └─ References: 0_TERMS_MEANING.md

Every document includes:
    • Back-link → 01_WORKFLOW_PIPELINE.md
    • "Related Documentation" section pointing to other docs
```

---

## Purpose & Topics

This directory contains detailed documentation of IRC server mechanisms that are essential for understanding and debugging the system. Topics focus on:
- **Architecture**: Layered design (I/O, Protocol, Logic, Application)
- **Flow**: Control flow through major subsystems
- **Mechanisms**: How specific features work internally
- **Concepts**: Networking concepts relevant to IRC protocol
