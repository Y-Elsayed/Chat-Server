#include <stdbool.h>
#include <string.h> // for strlen
#include <libpq-fe.h> // PostgreSQL header
#include <cJSON.h> // cJSON header
extern PGconn *db_conn;

void init_database();
void create_table();
ssize_t send_all(int sockfd, const void *buf, size_t len, int flags);
void send_chat_history(int client_socket);
void insert_message(const char * time,const char *username, const char *message);
void insert_username(const char *username);
bool verify_username(const char *username); //returns true if valid (unique).



/*
    cJSON *username = cJSON_GetObjectItem(root, "username");
    cJSON *message = cJSON_GetObjectItem(root, "message");
    cJSON *time = cJSON_GetObjectItem(root, "time");
*/