## packet Defininition

### packet Classifications

- BLANK(0): The packet is used to fill the blank.
- CONNECT(1): The packet is used to connect to the socket server.
- DISCONNECT(2): The packet is used to close the socket connection.
- REQTIME(3): The packet is used to request the time from the server.
- REQHOST(4): The packet is used to request the host's hostname from the server.
- REQCLI(5): The packet is used to request the list of all the clients connected to the server.
- REQSEND(6): The packet is used to request the server to forward a message to a client.
- ACK(7): The packet is used to send the ack between the client and the server.
- FWD(8): The packet is used to forward the message from the server to the client.

The packet is defined as follows:

``` text
0                 8                16               24               32
+----------------+----------------+----------------+----------------+
|          Packet  Index          |  Packet  Type  |  Sender    ID  |
+----------------+----------------+----------------+----------------+
|  Receiver  ID  |  Elements  Num |  Element1 Len  | Element1 Start |     ......     |
+----------------+----------------+----------------+----------------+
|     ......     |     ......     |     ......     |  Element1 End  |
+----------------+----------------+----------------+----------------+
|  Element2 Len  | Element2 Start |     ......     |     ......     |
+----------------+----------------+----------------+----------------+
|     ......     |     ......     |  ElementN End  |
+----------------+----------------+----------------+
```

For ID, server is always 0, and the client is 1, 2, 3, ... Maximum 255 clients.

### Sender & Receiver

Sender and Receiver class are used to encapsulate the sender and receiver methods.

TODO: EXCEED TIME CONSTRAINTS IN RECV?