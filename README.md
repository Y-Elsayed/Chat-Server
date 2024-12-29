# Chat-Server

This repository contains a **multithreaded group chat server** application implemented in C as part of an Operating Systems project. The server allows multiple clients to communicate simultaneously, with messages stored in a PostgreSQL database. The application demonstrates concepts of socket programming, multithreading, and database management.

## Features

- **Group Chat Functionality**
  - Supports up to **10 chat members** simultaneously.
  - Real-time message broadcasting to all active participants.

- **User Queue Management**
  - If the chat room is full, new users are added to a **queue**.
  - As space becomes available, queued users are automatically added to the room.

- **Chat History Access**
  - New users joining the room can see the **entire chat history**, ensuring they don’t miss earlier conversations.

- **Real-time Messaging**
  - Clients can send and receive messages in real-time.

- **Thread Safety**
  - Mutex locks ensure safe access to shared resources and maintain data integrity.

- **Username Management**
  - Verifies the uniqueness of usernames upon client connection.

## How It Works

1. Users connect to the chat server through a client application.
2. Messages sent by any participant are broadcasted to all active members in the room.
3. If the room is full, additional users are placed in a queue until a spot becomes available.
4. Once a user leaves, the first person in the queue joins the chat and sees the full chat history.
5. Messages are logged in a PostgreSQL database for persistence.

## Architecture

- **Server:** Handles client connections, manages chat history, and broadcasts messages.
- **Database:** Stores chat messages and client information for persistence.
- **Concurrency:** Each client interaction is managed by a separate thread.

### Project Overview Diagram
![image](https://github.com/user-attachments/assets/2c3d992c-dd1c-4c89-9159-8687ba76858f)

This use case illustrates a scenario with two senders (`Client 1` and `Client 2`). When `Client 1` sends a message followed by `Client 2`, their messages are stored in a queue in the order they were sent. Each message is then delivered to all group members except the sender. Simultaneously, the logs of these messages are saved to the chat history database.

## Results and Challenges

- The application was developed and debugged over 30+ hours.
- The server-side functionality is complete and logically sound.
- A persistent challenge remains in connecting the HTML front end with the server components.

## Prerequisites

- **PostgreSQL** for message storage.
- **C++ Compiler** with multithreading support.


## How to Run

1. Clone the repository:
   ```bash
   git clone https://github.com/Y-Elsayed/Chat-Server.git
   cd Chat-Server
2. Set up the PostgreSQL database
3.Compile the server code:
    ```bash
    g++ -pthread -o chat_server chat_server.cpp
4. Run the server
    ```bash
    ./chat_server
5. Connect clients to the server using the specified IP and port.

## Future Work
- Resolve the connection issue between the HTML front end and the server.
- Enhance error handling and logging mechanisms.
- Expand client features and improve user experience.
