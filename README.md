Grpc server client example

Client tasks:
 1. Sends keep active periodically
 2. Receive notification for power state change
 3. Acknowledge power state
 4. Exit on receiving ACTIVE state

Server tasks:
 1. Receive keep active
 2. Change internal state when keep active is received
 3. Notify about new state