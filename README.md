# C_messenger

A little project to explore network programming in C facilitating communication between multiple clients through a server leveraging TCP stream sockets and threads. This should only work for unix.

Currently the clients connect and communicate with the server successfully, however utilizing processes instead of threads makes the way file descriptors are implemented for each client makes broadcasting unsuccessful to peers.  
Todo:
- Change clients from processes to threads
- Allow customization of bound port and address.
- Add time and client information into broadcast messages for clarity
- Backend to store messages
