# Group Chat Server

A **simple group chat server** implemented in C. This project demonstrates basic socket programming concepts and manages a group chat room with limited capacity. Users can join the chat room, participate in real-time conversations, and access the full chat history.

## Features
- **Group Chat Functionality**  
  - Supports up to **10 chat members** simultaneously.
  - Real-time message broadcasting to all active participants.

- **User Queue Management**  
  - If the chat room is full, new users are added to a **queue**.
  - As space becomes available, queued users are automatically added to the room.

- **Chat History Access**  
  - New users joining the room can see the **entire chat history**, ensuring they don’t miss earlier conversations.

## How It Works
1. Users connect to the chat server through a client application.
2. Messages sent by any participant are broadcasted to all active members in the room.
3. If the room is full, additional users are placed in a queue until a spot becomes available.
4. Once a user leaves, the first person in the queue joins the chat and sees the full chat history.
