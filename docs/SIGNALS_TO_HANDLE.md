```markdown
# Signal Handling: Ctrl+\ (SIGQUIT)

**Yes, SIGQUIT must be handled.** The subject forbids any unexpected termination:

> *"Your program should not crash in any circumstances... and should not quit unexpectedly.
> If it happens, your grade will be 0."*

## Why Ctrl+\ Is Dangerous

`Ctrl+\` sends **SIGQUIT** (signal 3). Default behavior: terminate the process **and** produce a core dump —
exactly the "unexpected termination" the subject prohibits.
Both `signal` and `sigaction` are explicitly listed in allowed functions.

## Minimal Handler

```cpp
#include <csignal>

static volatile bool g_running = true;

static void signalHandler(int signum) {
    (void)signum;
    g_running = false; // exits the poll() loop cleanly
}

// in main(), before poll loop:
signal(SIGINT,  signalHandler); // Ctrl+C
signal(SIGQUIT, signalHandler); // Ctrl+\ — prevents core dump
```


## Signals to Handle

| Signal | Shortcut | Default Action | Handle? |
| :-- | :-- | :-- | :-- |
| SIGINT | Ctrl+C | Terminate | ✅ Yes |
| SIGQUIT | Ctrl+\ | Terminate + core dump | ✅ Yes |
| SIGPIPE | client drop | Terminate | ✅ Yes — `signal(SIGPIPE, SIG_IGN)` |
| SIGTERM | `kill <pid>` | Terminate | ✅ Recommended |

> **SIGPIPE is critical:** if a client disconnects during `send()`, the server crashes without handling it.
> The evaluation explicitly tests "unexpectedly kill a client."

```
```

### Ctrl+D just to explain myself

Imagine: nc is like a notepad, and you write the command NICK bob. Ctrl+D tells nc: “send everything you've typed right now,” without waiting for Enter/\n.

```
nc -C 127.0.0.1 6667
# type: com   → Ctrl+D  (sends "com")
# type: man   → Ctrl+D  (sends "man")
# type: d     → Enter   (sends "d\r\n")
```
The server must assemble three pieces into a single command\r\n. This checks MessageBuffer.

Ctrl+D is not dangerous for the server itself — the server does not read stdin, so it does not care. This signal is only for the nc process on the client side.


