# Computer Network - Socket Programming

## Introduction

This is a simple socket programming project for the course Computer Network. The project has implemented a simple socket server and client, which can be used to send messages between clients by relaying on the server using Non-blocking socket receiver and epoll.

## Usage

This project is developed and tested on Arch Linux (6.6.2-arch1-1) and Debian 12. MacOS and Windows are not supported since `epoll` is not supported on these platforms.

> ~~It seems epoll is not necessary for this project since the server can handle multiple clients by creating multiple threads. However, I still use epoll to implement the server since it is a good practice.~~
> Here I use `epoll` to poll the socket with some certain timeout in order to avoid the busy waiting while receiving the message non-blockingly.
> If you want to transfer the project to other platforms, you can try to ~~remove the epoll part (or~~ use `select` `poll` instead of `epoll` ~~)~~. It should work. :)
> Moreover, in this project, I use `future` in main function to wait for user's input. It can be replaced by `select` `poll` `epoll` to wait for both user's input and message queue.

<!-- see linux release version: `cat /etc/*-release` -->

### Server

``` bash
./server [host] [address] [port]    # Need to provide in sequence
```

### Client

``` bash
./client [host] [address] [port]    # Need to provide in sequence
```

After the client starts, it can be operated by the following commands:

``` bash
1. connect [<server address> <server port>]: Connect to the server.
        <server address>: The address of the server.
        <server port>: The port of the server.
        If not specified, the default address and port will be used.
2. disconnect: Disconnect from the server.
3. gettime: Get the time from the server.
4. gethost: Get the name of the server.
5. getcli: Get the list of the clients.
6. send <id> "<content>": Send a message to a client->
        <id>: The id of the receiver.
        <content>: The content of the message. Need to be quoted.
7. help: Print this message.
0. exit: Exit.
```

For example:

``` bash
send 2 "Hello World!"
```

## Implementation

### Packet Classifications

The packet is defined as follows:

``` text
0                 8                16               24               32
+----------------+----------------+----------------+----------------+
|          Packet  Index          |  Packet  Type  |  Sender    ID  |
+----------------+----------------+----------------+----------------+
|  Receiver  ID  |  Elements  Num |  Element1 Len  | Element1 Start |
+----------------+----------------+----------------+----------------+
|     ......     |     ......     |     ......     |  Element1 End  |
+----------------+----------------+----------------+----------------+
|  Element2 Len  | Element2 Start |     ......     |     ......     |
+----------------+----------------+----------------+----------------+
|     ......     |     ......     |  ElementN End  |
+----------------+----------------+----------------+
```

Package Type is defined as follows:

- BLANK(0): The packet is used to fill the blank.
  - NOT USED.
- CONNECT(1): The packet is used to connect to the socket server.
  - Sender ID is initialized to 0.
  - Receiver ID is initialized to Server ID (0).
  - No Element in the packet.
- DISCONNECT(2): The packet is used to close the socket connection.
  - If it is client request
    - Sender ID is self ID.
    - Receiver ID is Server ID (0).
    - No Element in the packet.
  - If it is server instruction
    - Sender ID is Server ID (0).
    - Receiver ID is target host ID.
    - No Element in the packet.
- REQTIME(3): The packet is used to request the time from the server.
  - Sender ID is self ID.
  - Receiver ID is Server ID (0).
  - No Element in the packet.
- REQHOST(4): The packet is used to request the host's hostname from the server.
  - Sender ID is self ID.
  - Receiver ID is Server ID (0).
  - No Element in the packet.
- REQCLI(5): The packet is used to request the list of all the clients connected to the server.
  - Sender ID is self ID.
  - Receiver ID is Server ID (0).
  - No Element in the packet.
- REQSEND(6): The packet is used to request the server to forward a message to a client.
  - Sender ID is self ID.
  - Receiver ID is target host ID.
    - Send the message to the server first to forward to the target host.
  - All the elements are the message to be sent.
- ACK(7): The packet is used to send the ack between the client and the server.
  - Sender ID is self ID.
  - Receiver ID is target host ID.
  - Can carry data.
    - ACK to REQTIME
      - A string of the timestamp.
    - ACK to REQHOST
      - A string of the hostname.
    - ACK to REQCLI
      - k elements in the packet, where k is the number of clients.
      - Every host is presented by 1 elements in the following format:
        - `Client_ID#Client_name#Client_IP#Client_port#`
        - `#` represents a chosen separator in `include/def.hpp`
    - ACK to REQSEND
      - If the message is sent successfully, the packet contains no data.
      - Else, the packet contains the error message.
    - ACK to CONNECT
      - No data.
    - ACK to DISCONNECT
      - No data.
    - ACK to FWD
      - No data.
- FWD(8): The packet is used to forward the message from the server to the client.
  - Same as REQSEND, but
    - change Package Index to the current index of Server
    - change Package Type to FWD

For ID, server is always 0, and the client is 1, 2, 3, ... Maximum 255 clients.

### Sender & Receiver

Sender and Receiver class are used to encapsulate the sender and receiver methods. We simply view them as 2 hosts here.

#### Connect

When connect, the client will send a CONNECT packet to the server. The server will add the client to the client list and send an ACK packet to the client. The client will receive the ACK packet and change self ID to the ID assigned by the server.

``` text
+------------+        CONNECT        +------------+
|   Client   | --------------------> |   Server   |
+------------+                       +------------+
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
+------------+          ACK          +------------+
|   Client   | <-------------------- |   Server   |
+------------+                       +------------+
```

#### Disconnect

When client requests disconnect, the client will send a DISCONNECT packet to the server. The server will remove the client from the client list and send an ACK packet to the client. The client will receive the ACK packet and close socket and change self ID to -1 to indicate that the socket is closed.

``` text
+------------+       DISCONNECT      +------------+
|   Client   | --------------------> |   Server   |
+------------+                       +------------+
                                       Close conn
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
+------------+          ACK          +------------+
|   Client   | <-------------------- |   Server   |
+------------+                       +------------+
  Close conn
```

If the server gives a client a DISCONNECT packet, the client will close the socket and change socket status to `-1` to indicate that the socket is closed. Also, an ACK packet will be sent to the server. When the server receives the ACK packet, the server will remove the client from the client list.

``` text
+------------+       DISCONNECT      +------------+
|   Client   | <-------------------- |   Server   |
+------------+                       +------------+
  Close conn
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
+------------+          ACK          +------------+
|   Client   | --------------------> |   Server   |
+------------+                       +------------+
                                       Close conn
```

#### Request Time

When client requests time, the client will send a REQTIME packet to the server. The server will send an ACK packet carrying the timestamp to the client. The client will receive the ACK packet and print the timestamp.

``` text
+------------+         REQTIME       +------------+
|   Client   | --------------------> |   Server   |
+------------+                       +------------+
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
+------------+          ACK          +------------+
|   Client   | <-------------------- |   Server   |
+------------+   carrying timestamp  +------------+
```

#### Request Hostname

When client requests hostname, the client will send a REQHOST packet to the server. The server will send an ACK packet carrying the hostname to the client. The client will receive the ACK packet and print the hostname.

``` text
+------------+        REQHOST        +------------+
|   Client   | --------------------> |   Server   |
+------------+                       +------------+
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
+------------+          ACK          +------------+
|   Client   | <-------------------- |   Server   |
+------------+   carrying hostname   +------------+
```

#### Request Client List

When client requests client list, the client will send a REQCLI packet to the server. The server will send an ACK packet carrying the client list to the client. The client will receive the ACK packet and print the client list.

``` text
+------------+         REQCLI        +------------+
|   Client   | --------------------> |   Server   |
+------------+                       +------------+
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
+------------+          ACK          +------------+
|   Client   | <-------------------- |   Server   |
+------------+  carrying client list +------------+
```

#### Request Send Message

When client requests send message, the client will send a REQSEND packet to the server. The server will send an ACK packet to the client. The client will receive the ACK packet and print the message.

``` text
+------------+        REQSEND        +------------+                       +------------+
|  Client 1  | --------------------> |   Server   | --------------------- |  Client 2  |
+------------+        message        +------------+                       +------------+
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
+------------+                       +------------+          FWD          +------------+
|  Client 1  | --------------------- |   Server   | --------------------> |  Client 2  |
+------------+                       +------------+        message        +------------+
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
+------------+                       +------------+          ACK          +------------+
|  Client 1  | --------------------- |   Server   | <-------------------- |  Client 2  |
+------------+                       +------------+                       +------------+
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
+------------+          ACK          +------------+                       +------------+
|  Client 1  | <-------------------- |   Server   | --------------------- |  Client 2  |
+------------+                       +------------+                       +------------+
```

## Declaration

ONLY FOR REFERENCE. Please **DO NOT COPY** the code **DIRECTLY**.
