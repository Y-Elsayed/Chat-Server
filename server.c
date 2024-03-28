#include "database.h" //Database functions

#define PORT 8080
#define MAX_CLIENTS 10
int client_sockets[MAX_CLIENTS];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void printIPAddress(int port) {
    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);
    struct hostent *host_info;
    host_info = gethostbyname(hostname);
    char *ip_address = inet_ntoa(*(struct in_addr *)host_info->h_addr_list[0]);
    printf("Server running at IP: %s, Port: %d\n", ip_address, port);
}

//Attempt at multithreading
void *handle_client(void *cs);

int try_bind_alternative_addresses(int server_fd, struct sockaddr_in *address);

int main()
{
    pthread_mutex_init(&mutex, NULL);
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
    if (try_bind_alternative_addresses(server_fd, &address) != 0) {
        printf("Failed to bind to any alternative IP addresses. Exiting.\n");
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

        // Set client sockets array to associated cs values.
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

            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (client_sockets[i] == 0)
                {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
            // Add new socket to array of client sockets
            pthread_t thread_id; // Create a new thread for the client
            if (pthread_create(&thread_id, NULL, handle_client, (void *)&new_socket) != 0)
            {
                perror("pthread_create");
                // Handle thread creation error
            }
            pthread_detach(thread_id);
            /*Might add it in the handle_client function, but should make sure to use mutexes for synchronization*/
        }
    }

    return 0;
}
void *handle_client(void *cs)
{
    int client_socket = *((int *)cs);
    char client_buffer[1024];
    ssize_t bytes_received;

    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    // Receive username (Obligatory) with a timeout
    char username_buffer[256];
    memset(username_buffer, 0, sizeof(username_buffer)); // Clear username buffer

    fd_set readfds;
    struct timeval timeout;
    timeout.tv_sec = 10; // Set timeout to 30 seconds
    timeout.tv_usec = 0;

    FD_ZERO(&readfds);
    FD_SET(client_socket, &readfds);

    int activity = select(client_socket + 1, &readfds, NULL, NULL, &timeout);
    if (activity == -1)
    {
        perror("select");
        close(client_socket);
        pthread_exit(NULL);
    }
    else if (activity == 0)
    {
        // Timeout occurred
        printf("Timeout occurred while waiting for username.\n");
        close(client_socket);
        pthread_exit(NULL);
    }
    else
    {
        // Wait to read the username:
        if ((bytes_received = recv(client_socket, username_buffer, sizeof(username_buffer) - 1, 0)) <= 0)
        {
            // Failed to receive username or client disconnected
            printf("Failed to receive username or client disconnected.\n");
            close(client_socket);
            pthread_exit(NULL);
        }
        else
        {
            // Null-terminate the received data to make it a valid C string
            username_buffer[bytes_received] = '\0';

            cJSON *root_username = cJSON_Parse(username_buffer);
            if (root_username == NULL)
            {
                // Handle parsing error
                printf("Failed to receive username or client disconnected.\n");
                close(client_socket);
                pthread_exit(NULL);
            }
            cJSON *username_item = cJSON_GetObjectItem(root_username, "username");
            const char *username = username_item->valuestring;
            printf("Username received: %s\n", username); // might send this as a message to the front-end
            cJSON_Delete(root_username);
            // pthread_mutex_lock(&mutex);
            // insert_username(username);
            // pthread_mutex_unlock(&mutex);
            /*if (!insert_username(username))
            {
                send a message to the UI in welcome.html telling the client to use another username and goto "again".
            }*/
            // Send chat history to the client upon connection
            send_chat_history(client_socket);
            // Handle communication with the client
            while ((bytes_received = recv(client_socket, client_buffer, sizeof(client_buffer), 0)) > 0)
            {
                client_buffer[bytes_received] = '\0';
                cJSON *root_msg = cJSON_Parse(client_buffer);

                // Extract fields from the JSON object
                cJSON *timestamp_item = cJSON_GetObjectItem(root_msg, "time");
                const char *timestamp = (timestamp_item != NULL) ? timestamp_item->valuestring : "";
                cJSON *message_item = cJSON_GetObjectItem(root_msg, "message");
                const char *message = (message_item != NULL) ? message_item->valuestring : "";

                // Broadcast only the message to other clients
                for (int j = 0; j < MAX_CLIENTS; j++)
                {
                    int dest_sd = client_sockets[j];
                    if (dest_sd != 0 && dest_sd != client_socket)
                    {
                        // send(dest_sd, client_buffer, bytes_received, 0);
                        send(dest_sd, message, strlen(message), 0);
                    }
                }
                // Message sent to all other clients, now it is safe to insert it into database.
                pthread_mutex_lock(&mutex);
                insert_message(timestamp, username, client_buffer);
                pthread_mutex_unlock(&mutex);
                // Free cJSON object
                cJSON_Delete(root_msg);
            }
            // Client disconnected
            // Clean up resources and close the socket
            getpeername(client_socket, (struct sockaddr *)&address, &addrlen);
            printf("Client disconnected, ip %s, port %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
            close(client_socket);
            // Remove the client socket from the array
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (client_sockets[i] == client_socket)
                {
                    pthread_mutex_lock(&mutex);
                    client_sockets[i] = 0;
                    pthread_mutex_unlock(&mutex);
                    break;
                }
            }
            pthread_exit(NULL);
        }
    }
}
int try_bind_alternative_addresses(int server_fd, struct sockaddr_in *address) {
    struct hostent *host_info;
    struct sockaddr_in *s;
    char hostname[256];
    char ip[256];
    int num_addresses = 0;
    int i;

    // Get the hostname
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("gethostname");
        return -1;
    }

    // Get host information
    if ((host_info = gethostbyname(hostname)) == NULL) {
        perror("gethostbyname");
        return -1;
    }

    // Iterate through the list of addresses and attempt to bind
    for (i = 0; host_info->h_addr_list[i] != NULL; i++) {
        s = (struct sockaddr_in *)host_info->h_addr_list[i];
        const char *ip_address = inet_ntop(AF_INET, &(s->sin_addr), ip, sizeof(ip));
        if (ip_address == NULL) {
            perror("inet_ntop");
            return -1;
        }

        printf("Trying to bind to IP address: %s\n", ip);

        // Set the IP address
        address->sin_addr.s_addr = inet_addr(ip);

        // Attempt to bind
        if (bind(server_fd, (struct sockaddr *)address, sizeof(*address)) == 0) {
            printf("Successfully bound to IP address: %s\n", ip);
            return 0; // Successful bind
        }

        perror("Bind Failed");
    }

    return -1; // All attempts failed
}