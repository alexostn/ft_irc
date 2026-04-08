# IPV6_V6ONLY flag

```
You have full read access to this C++ project (ft_irc, C++98).

Search the entire codebase for any socket created with AF_INET6.
For each one, check whether IPV6_V6ONLY is explicitly set via setsockopt().

────────────────────────────────────────────
WHAT TO LOOK FOR
────────────────────────────────────────────

CORRECT — explicit and cross-platform safe:
    int no = 0;
    setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no));
    // value = 0 → dual-stack: accepts both IPv4 and IPv6 clients
    // must appear immediately after socket(AF_INET6, ...) call

WRONG — missing setsockopt entirely:
    serverSocketFd_ = socket(AF_INET6, SOCK_STREAM, 0);
    // no IPV6_V6ONLY call → behaviour depends on OS default:
    //   Linux default = 1 → IPv6 only, IPv4 clients rejected
    //   macOS default = 0 → dual-stack works by accident
    // same binary behaves differently on different platforms

WRONG — set to 1:
    int yes = 1;
    setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &yes, sizeof(yes));
    // explicitly IPv6-only, IPv4 clients will be rejected

────────────────────────────────────────────
IF AF_INET IS USED (not AF_INET6)
────────────────────────────────────────────

IPV6_V6ONLY does not apply to AF_INET sockets at all.
If the project uses socket(AF_INET, ...) only → report "not applicable,
project is IPv4-only, no IPV6_V6ONLY needed".

────────────────────────────────────────────
REPORT FORMAT
────────────────────────────────────────────

For each socket() call found, report:
  - file and line number
  - address family used (AF_INET / AF_INET6)
  - whether IPV6_V6ONLY setsockopt is present (yes / no / not applicable)
  - current value set (0 = dual-stack / 1 = IPv6-only / missing)
  - verdict: CORRECT / NEEDS FIX / NOT APPLICABLE

Then give a one-line overall verdict:
  "All sockets correctly configured" OR
  "Fix required in <file>:<line> — reason"
```
___________________________________________________________________________

`IPV6_V6ONLY = 0` tells the kernel: “This `AF_INET6` socket accepts **both** protocols.” An IPv4 client arrives with an address in IPv4-mapped format: `::ffff:192.168.1.5`. The kernel handles the conversion automatically—you see all clients through a single `accept()`.

```
IPV6_V6ONLY = 0  →  one socket, both protocols
IPV6_V6ONLY = 1  →  one socket, IPv6 only
AF_INET          →  one socket, IPv4 only (flag not applicable at all)
```

Schematically:

```
IPv4 client (192.168.1.5)  ──┐
                              ├──▶  socket(AF_INET6) + IPV6_V6ONLY=0
IPv6 client (::1)          ──┘
```

All that is needed for dual-stack is these two conditions together:

```cpp
socket(AF_INET6, SOCK_STREAM, 0)   // ← AF_INET6, not AF_INET
int no = 0;
setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no));  // ← explicitly set to 0
```

Without the first, there is no IPv6. Without the second, there is no IPv4 on Linux (default `= 1`). Both conditions are mandatory.