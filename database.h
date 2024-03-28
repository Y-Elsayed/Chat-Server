#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <libpq-fe.h> // PostgreSQL header
#include <cJSON.h> // cJSON header
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>   // Add for errno
#include <pthread.h>
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