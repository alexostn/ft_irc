# Handle the PASS → NICK → USER order strictly and track the client registration state at each step.

# Allow clients to retry PASS on password mismatch instead of disconnecting immediately, with a limited number of attempts.

# Treat nicknames as case-insensitive (e.g. BOB and bob must resolve to the same user) while preserving the original casing for display.

# Correctly handle missing or empty PASS/NICK/USER parameters and send the appropriate IRC error numerics.

Here is quote in case I got smth wrong:
"Configuring the client is quite short, but had some tricky elements.In halloys case It should have the port and address of the server you want to connect to.But it can also include a nickname, username, realname, and password in the config file which the client should automatically forward to the server upon connection.You should also handle cases where there is no nickname, username, realname, or password in the config file, as well as cases where the password was entered incorrectly and the client should retry entering the password. You also should be careful with case sensitivity. Nicknames are not case sensitive in IRC meaning BOB and bob should be recognised as the same user
There is a lot of edge cases and these I've listed are just a few
You'll have to dig through documentation to really do IRC properly"
