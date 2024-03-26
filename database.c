#include <json-c/json.h>

void init_database() {
    // Connect to the PostgreSQL database given its name and the user/password of its creator. Note that the connection stays.
    db_conn = PQconnectdb("dbname=chat_history user=your_username password=your_password");
    //Check that the connection was successful, otherwise pop an error.
    if (PQstatus(db_conn) != CONNECTION_OK) {
        fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(db_conn));
        PQfinish(db_conn);
        exit(EXIT_FAILURE);
    }
    // Create the necessary table if it doesn't exist
    create_table();
}
void create_table() {
    //SQL command to be run, written as raw string.
    const char *sql = "CREATE TABLE IF NOT EXISTS Messages (" // Don't create the table if it does exist.
                      "Timestamp TIMESTAMP PRIMARY KEY," //Inserted with special PSQL value CURRENT_TIMESTAMP.
                      "Username TEXT,"
                      "Content TEXT);";

    PGresult *res = PQexec(db_conn, sql); // Excute the SQL query using the previously established connection.
    //Error Handling if the execution failed.
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Failed to create table: %s", PQerrorMessage(db_conn));
        PQclear(res);
        exit(EXIT_FAILURE);
    }
    PQclear(res); // Free Memory.
}
//Function to send a specific message (all contained within the buffer) over a socket
//Send all is better than send in that it doesn't get interrupted, sends all that is within the buffer.
ssize_t send_all(int sockfd, const void *buf, size_t len, int flags) { //Parameters: socket file descriptor, buffer to read and send from, size of buffer in bytes, additional modification.
    size_t total_sent = 0; //Total number of bytes sent
    //While there are more bytes to be sent.
    while (total_sent < len) {
        ssize_t sent = send(sockfd, buf + total_sent, len - total_sent, flags); //Try to send remaining bytes to client
        if (sent == -1) { //Send failed
            if (errno == EINTR) { // Interrupted by signal, retry
                continue;
            } else { // Error occurred, return -1 to indicate failure
                return -1;
            }
        }
        if (sent == 0) { // Connection closed by peer, return total bytes sent
            //Could do something different here.
            return total_sent;
        }
        //Otherwise, we have successfully sent bytes, increment them to counter.
        total_sent += sent;
    }
    // All data sent successfully
    return total_sent;
}
// Function to retrieve chat history and send it to the client
void send_chat_history(int client_socket) {
    const char *sql = "SELECT * FROM Messages ORDER BY Timestamp ASC;"; //Query to get all of the chat currently.
    PGresult *res = PQexec(db_conn, sql); // Execute query through connection.
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Failed to retrieve sorted messages: %s", PQerrorMessage(db_conn));
        PQclear(res);
        exit(EXIT_FAILURE);
    }

    // Iterate through the result and send each message to the client
    int num_rows = PQntuples(res);
    //Iterate over each tuple, get the column values for each to add them to json object then send messages to the client consequently.
    for (int i = 0; i < num_rows; ++i) {
        json_object *message_obj = json_object_new_object();
        json_object_object_add(message_obj, "timestamp", json_object_new_string(PQgetvalue(res, i, 0)));
        json_object_object_add(message_obj, "username", json_object_new_string(PQgetvalue(res, i, 1)));
        json_object_object_add(message_obj, "content", json_object_new_string(PQgetvalue(res, i, 2)));

        // Convert the JSON object to a string
        const char *json_str = json_object_to_json_string(message_obj);

        // Send the JSON string to the client
        send_all(client_socket, json_str, strlen(json_str), 0);

        // Free the JSON object
        json_object_put(message_obj);
    }
    PQclear(res);
}

// Modify the handle_client function to handle chat history requests
void *handle_client(void *clientInfo) {
    client_info *client = (client_info *)clientInfo; //Point to given client info.
    char buffer[MAX_MSG_SIZE]; //Memory buffer to store incoming messages.
    ssize_t bytes_received; //Counter for the bytes received
    //Similar to send, as recv returns 0 (connection closed) keep looping, and it should return zero when it finished sending.
    while ((bytes_received = recv(client->socket, buffer, MAX_MSG_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0'; // Null terminate the received message (To close string)
        if (strcmp(buffer, "GET_HISTORY") == 0) {
            // Client requested chat history
            send_chat_history(client->socket);
        } else { //Else should mean they are trying sending a message, we need to send that message.

            // Process other messages (e.g., save to database, broadcast to other clients)
            printf("Received message from %s: %s\n", inet_ntoa(client->address.sin_addr), buffer);
            // Echo back to the client
            send(client->socket, buffer, strlen(buffer), 0);
        }
    }
    //We reach here after they
    printf("Client disconnected: %s\n", inet_ntoa(client->address.sin_addr));
    close(client->socket);
    free(client);
    pthread_exit(NULL);
}