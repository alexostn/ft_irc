# How to join Halloy to our Server

## Install Halloy
For Linux:
```bash
sudo snap install halloy
```

## Set password, nick and user

1. Click option menu at the bottom left corner

2. Choose "Open config file"

3. Add following code:
```
[servers.ft_irc]
server = "127.0.0.1"
port = 6667
use_tls = false
tls_verify = false
password = "test123"
nickname = "art"
username = "Artur"
realname = "Artur The 42 Warsaw Student"
```

where:
- server has to be "127.0.0.1" (localhost)
- port is the port used by our server (provided when starting server)
- password is the PASS that is required by our server (provided when starting server)
- nickname is the NICK that will be used in server
- username is the USER that will be used in server
- realname can be whatever

4. Save

5. Again, click option menu at the bottom left corner

6. Choose "Reload config file"

7. Our ft_irc should appear on the left pane of Halloy

## Try some commands

When using commands in Halloy, use "/" just before command, e.g. "/JOIN #testchannel"