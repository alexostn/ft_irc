# `inet_ntoa` vs `inet_ntop`

Both convert a binary IP address into a human-readable string.
The core difference is **who owns the buffer**.

---

## The core difference

```cpp
// inet_ntoa — returns pointer to a static buffer INSIDE the function
char* ip = inet_ntoa(addr.sin_addr);   // you don't own this memory

// inet_ntop — writes into YOUR buffer
char buf[INET6_ADDRSTRLEN];
inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf));  // you own this
```

With `inet_ntoa`, the second call overwrites the first:

```cpp
char* ip1 = inet_ntoa(addr1.sin_addr);  // "192.168.1.1"
char* ip2 = inet_ntoa(addr2.sin_addr);  // "192.168.1.2"

printf("%s\n", ip1);  // prints "192.168.1.2" — wrong, buffer was overwritten
printf("%s\n", ip2);  // prints "192.168.1.2"
```

With `inet_ntop`, results are independent:

```cpp
char ip1[INET_ADDRSTRLEN];   // 16 bytes — enough for IPv4
char ip2[INET_ADDRSTRLEN];

inet_ntop(AF_INET, &addr1.sin_addr, ip1, sizeof(ip1));
inet_ntop(AF_INET, &addr2.sin_addr, ip2, sizeof(ip2));

printf("%s\n", ip1);  // "192.168.1.1" — correct
printf("%s\n", ip2);  // "192.168.1.2" — correct
```

---

## Comparison

| | `inet_ntoa` | `inet_ntop` |
|---|---|---|
| Buffer | static, inside the function | yours, passed as argument |
| Multiple calls safe | no — each call overwrites | yes — independent buffers |
| Thread-safe | no | yes |
| IPv4 | yes | yes |
| IPv6 | no | yes |
| Status | deprecated | current standard |

---

## Name breakdown

```
inet_ntoa  →  inet  +  n  +  to  +  a
                         │          └─ ASCII
                         └─ network (binary)

inet_ntop  →  inet  +  n  +  to  +  p
                         │          └─ presentation (human-readable)
                         └─ network (binary)
```

The only conceptual change: `a` (ASCII, implicit static buffer) became
`p` (presentation, explicit buffer you provide).

---

## Buffer size constants

```cpp
#include <arpa/inet.h>

INET_ADDRSTRLEN   // 16  — max IPv4 string: "255.255.255.255\0"
INET6_ADDRSTRLEN  // 46  — max IPv6 string: "ffff:ffff:...%scope\0"
```

Always use these constants instead of magic numbers.

---

## Rule of thumb

Use `inet_ntop`. Always. `inet_ntoa` is kept only for legacy code compatibility.
