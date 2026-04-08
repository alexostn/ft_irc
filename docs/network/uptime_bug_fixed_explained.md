```markdown
# Bug Fix: `!uptime` Returns Raw Seconds Instead of Formatted Time

**Symptom:** `uptime: 4292s`  
**Expected:** `uptime: 1h 11m 32s`

---

## Root Cause

Raw arithmetic result was cast directly to string without formatting.

## Fix

```cpp
// BEFORE (broken)
"uptime: " + to_str(time(NULL) - start_time) + "s"

// AFTER (correct)
std::string format_uptime(time_t start) {
    long s = (long)(time(NULL) - start);
    return to_str(s / 3600) + "h " +
           to_str((s % 3600) / 60) + "m " +
           to_str(s % 60) + "s";
}

"uptime: " + format_uptime(start_time)
```

## Docs Update

In the commands reference, update the `!uptime` example response:

| Before 		  | After 				 |
|-----------------|----------------------|
| `uptime: 4292s` | `uptime: 1h 11m 32s` |

> **Note:** `start_time` is set once at `main()` startup and is never reset on reconnect.
```