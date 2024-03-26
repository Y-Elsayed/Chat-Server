#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>   // Add for errno
#include <stdbool.h> // Add for boolean type
#include <pthread.h>

#define PORT 8080
#define MAX_CLIENTS 10
client_sockets[MAX_CLIENTS];

int main()
{
    /*
        - server_fd: This variable represents the file descriptor for the server socket. It is used to accept incoming connections and manage communication with clients.
        - client_sockets[MAX_CLIENTS]: This array is used to store file descriptors for client sockets. Each element of the array corresponds to a connected client. When a new client connects, its socket descriptor is stored in an available slot in this array.
        - new_socket: This variable holds the file descriptor for a newly accepted client connection. It is used to communicate with the specific client.
        - max_sd: This variable tracks the maximum file descriptor value among all active sockets (both server and client sockets). It is used as an argument to the select() function to determine the highest descriptor to check for activity.
        - activity: This variable stores the return value of the select() function, indicating the number of sockets with activity (readiness for reading) detected.
        - address: This variable is of type struct sockaddr_in and represents the server's address. It is used for binding the server socket to a specific IP address and port.
        - readfds: This variable is of type fd_set and represents the set of file descriptors to be monitored for activity by the select() function. It is initialized and updated before each call to select().
        - buffer[1024]: This array is used to store data received from clients. It serves as a buffer for incoming messages.
        - opt: This variable is used to set socket options, such as SO_REUSEADDR and SO_REUSEPORT. It allows multiple sockets to bind to the same address and port.
        - addrlen: This variable of type socklen_t is used to store the size of the address structure (struct sockaddr_in) when calling functions like accept(). It is passed as a pointer to accept() to retrieve the actual size of the client's address structure.
    */
    int server_fd, new_socket, max_sd, activity;
    struct sockaddr_in address;
    fd_set readfds;
    char server_buffer[1024] = {0};
    int opt = 1;
    socklen_t addrlen; // Add socklen_t addrlen;

    // Create a master socket
    /*Master socket is the main socket responsible for accepting incoming client requests to the servre
    when a new connection is accepted, a new socket is created to handle it and this master socket stays
    listening(waiting) for another connection
    */
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options to allow multiple connections
    /*This function sets the socket properties
      it takes the socket file descriptor as argument to know which socket to change
    - SOL_SOCKET: This specifies the level at which the option is defined. SOL_SOCKET indicates that the option is at the socket level.
    - SO_REUSEADDR: This is the socket option you want to set. It enables the reuse of local addresses. When this option is set,
      it allows other sockets to bind to the same port, even if the socket is in a TIME_WAIT state (waiting for packets to expire after closing).
    - &opt: it's a pointer to the variable opt, which likely has a value of 1 to enable the option.
    */
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // sets the socket address structure's family to use IPV4 protocol
    address.sin_family = AF_INET;

    /*This line sets the IP address of the socket to INADDR_ANY. When binding a socket to this address,
    the socket can receive packets destined for any of the IP addresses assigned to the host. It allows the socket to accept connections on any available network interface.*/
    address.sin_addr.s_addr = INADDR_ANY;

    /*This line sets the port number of the socket to PORT. Before assigning the port number,
    the htons() function is used to convert the port number from host byte order to network byte order.
    This conversion is necessary because different systems may use different byte orders (big-endian or little-endian),
    and network protocols require a consistent byte order for port numbers. By using htons(), the port number is appropriately
    formatted for use in network communication.*/
    address.sin_port = htons(PORT);

    /*
    Overall, this code snippet ensures that the server socket is successfully bound to the specified address and port.
    If the binding operation fails, an error message is printed, and the program exits with a failure status.
    This is crucial for error handling and ensuring that the server can properly initialize and start listening for incoming connections.
    */
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections, this returns the number of connections which will be -1 in case of error, and positive with the number of incoming connections
    // MAX_CLIENTS is the max length of the queue of pending connections
    if (listen(server_fd, MAX_CLIENTS) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Accept incoming connections and handle chat logic
    while (true)
    {
        /*
        - FD_ZERO(&readfds): This macro initializes the file descriptor set readfds to be empty. It clears all file descriptors from the set,
        ensuring that it starts with no sockets registered for monitoring.

        - FD_SET(server_fd, &readfds): This macro adds the server socket (server_fd) to the file descriptor set readfds.
        It indicates to the select() function that it should monitor the server socket for any activity, such as incoming connections.

        - max_sd = server_fd: This line sets the variable max_sd to the value of the server socket descriptor (server_fd).
        This variable is typically used to keep track of the maximum socket descriptor value among all sockets being monitored.
        It's initialized with the server socket descriptor because it's the only socket being monitored at this point.
        */
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            int sd = client_sockets[i];
            if (sd > 0)
            {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd)
            {
                max_sd = sd;
            }
        }

        /*When a client is trying to join the server*/
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Interested in reading so we passed the readfds and the write,error and timeout are nulls, since timeout is null it will wait infinitely till it catches a read request
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR))
        {
            perror("select error");
        }

        if (FD_ISSET(server_fd, &readfds)) // check if there is any new connection requests
        {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            printf("New connection, socket fd is %d, IP is: %s, port : %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            pthread_t thread_id;  // Create a new thread for the client
            if (pthread_create(&thread_id, NULL, handle_client, (void *)&new_socket) != 0)//should Implement handle_client now.
            {
                perror("pthread_create");
                // Handle thread creation error
            }
            pthread_detach(thread_id);

            // Add new socket to array of client sockets
            /*Might add it in the handle_client function, but should make sure to use mutexes for synchronization*/
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (client_sockets[i] == 0)
                {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        /* Newly added code for chat history*/

        // Handle incoming messages
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            int sd = client_sockets[i];

            if (FD_ISSET(sd, &readfds)) // check if socket is in the sockets set
            {
                // Check socket sd for data, if there is write it to server_buffer.
                if (read(sd, server_buffer, 1024) == 0) // Where message is sent.
                {
                    // lock buffer here
                    // Client disconnected here.
                    getpeername(sd, (struct sockaddr *)&address, &addrlen);
                    printf("Client disconnected, ip %s, port %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    close(sd);
                    client_sockets[i] = 0;
                }
                // else if there is a write request (write this condition)
                else
                {
                    // Broadcast message to other clients (forcfully), without need to
                    for (int j = 0; j < MAX_CLIENTS; j++)
                    {
                        int dest_sd = client_sockets[j];
                        if (dest_sd != 0 && dest_sd != sd)
                        {
                            send(dest_sd, server_buffer, strlen(server_buffer), 0);
                        }
                    }
                    /*Insert the msg in the db*/
                }
            }
        }
    }

    return 0;
}

// Modify the handle_client function to handle chat history requests
void *handle_client(void *clientInfo)
{
    client_info *client = (client_info *)clientInfo; // Point to given client info.
    char buffer[MAX_MSG_SIZE];                       // Memory buffer to store incoming messages.
    ssize_t bytes_received;                          // Counter for the bytes received
    // Similar to send, as recv returns 0 (connection closed) keep looping, and it should return zero when it finished sending.
    while ((bytes_received = recv(client->socket, buffer, MAX_MSG_SIZE - 1, 0)) > 0)
    {
        buffer[bytes_received] = '\0'; // Null terminate the received message (To close string)
        if (strcmp(buffer, "GET_HISTORY") == 0)
        {
            // Client requested chat history
            send_chat_history(client->socket);
        }
        else
        { // Else should mean they are trying sending a message, we need to send that message.

            // Process other messages (e.g., save to database, broadcast to other clients)
            /*Handle locks*/
            printf("Received message from %s: %s\n", inet_ntoa(client->address.sin_addr), buffer);
            // Echo back to the client
            send(client->socket, buffer, strlen(buffer), 0);
        }
    }
    // We reach here after they
    printf("Client disconnected: %s\n", inet_ntoa(client->address.sin_addr));
    close(client->socket);
    free(client);
    pthread_exit(NULL);
}