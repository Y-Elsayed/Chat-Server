#include "database.h"

PGconn *db_conn = NULL;

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
                      "Timestamp TIMESTAMP WITH TIME ZONE," //Inserted with special PSQL value CURRENT_TIMESTAMP.
                      "Username TEXT PRIMARY KEY,"
                      "Content TEXT);
                       CREATE TABLE IF NOT EXISTS Usernames (
                       Username TEXT PRIMARY KEY);";

    PGresult *res = PQexec(db_conn, sql); // Excute the SQL query using the previously established connection.
    //Error Handling if the execution failed.
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Failed to create table: %s", PQerrorMessage(db_conn));
        PQclear(res);
        exit(EXIT_FAILURE);
    }
    PQclear(res); // Free Memory.
}
// Function to insert a message into the Messages table
void insert_message(const char * time,const char *username, const char *message) {
    // Construct the SQL query for inserting a message, with the current timestamp of execution.
    const char *sql = "INSERT INTO Messages (Timestamp, Username, Content) VALUES ( $1, $2 , $3);";

    // Set up the parameter values for the query
    const char *paramValues[2] = {time,username, message};

    // Execute the SQL query with parameters
    PGresult *res = PQexecParams(db_conn, sql, 3, NULL, paramValues, NULL, NULL, 0);

    // Check if the query was successful
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Failed to insert message: %s", PQerrorMessage(db_conn));
    }

    // Clear the result
    PQclear(res);
}
// Function to insert a username into the Usernames table
void insert_username(const char *username) {
    // Construct the SQL query for inserting a message, with the current timestamp of execution.
    const char *sql = "INSERT INTO Usernames (Username) VALUES ($1);";

    // Set up the parameter values for the query
    const char *paramValues[1] = {username};

    // Execute the SQL query with parameters
    PGresult *res = PQexecParams(db_conn, sql, 1, NULL, paramValues, NULL, NULL, 0);

    // Check if the query was successful
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Failed to insert message: %s", PQerrorMessage(db_conn));
    }

    // Clear the result
    PQclear(res);
}
// Function to verify that the inserted username is unique
bool verify_username(const char *username)
{
    // Check if the database connection is valid
    if (!db_conn)
    {
        fprintf(stderr, "Database connection is not initialized.\n");
        return false;
    }

    // Construct the SQL query to check if the username already exists
    const char *sql = "SELECT Username FROM Usernames WHERE Username = $1;";
    const char *paramValues[1] = {username};

    // Execute the SQL query with parameters
    PGresult *res = PQexecParams(db_conn, sql, 1, NULL, paramValues, NULL, NULL, 0);

    // Check if the query was successful
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "Failed to execute query: %s", PQerrorMessage(db_conn));
        PQclear(res);
        return false;
    }

    // Check if any rows were returned (username already exists)
    bool exists = (PQntuples(res) > 0);

    // Clear the result
    PQclear(res);

    // Return true if the username does not exist (unique), false otherwise
    return !exists;
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
    const char *sql = "SELECT * FROM Messages ORDER BY Timestamp ASC;"; // Query to get all of the chat currently.
    PGresult *res = PQexec(db_conn, sql); // Execute query through connection.
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Failed to retrieve sorted messages: %s", PQerrorMessage(db_conn));
        PQclear(res);
        exit(EXIT_FAILURE);
    }
    // Iterate through the result and send each message to the client
    int num_rows = PQntuples(res);
    for (int i = 0; i < num_rows; ++i) {
        // Create a cJSON object for the message
        cJSON *message_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(message_obj, "timestamp", PQgetvalue(res, i, 0));
        cJSON_AddStringToObject(message_obj, "username", PQgetvalue(res, i, 1));
        cJSON_AddStringToObject(message_obj, "message", PQgetvalue(res, i, 2));

        // Convert the cJSON object to a JSON string
        const char *json_str = cJSON_PrintUnformatted(message_obj);

        // Send the JSON string to the client
        send_all(client_socket, json_str, strlen(json_str), 0);

        // Free the cJSON object
        cJSON_Delete(message_obj);
        free(json_str); // Free the allocated memory for the JSON string
    }
    PQclear(res);
}