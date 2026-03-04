#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "buffer.h"
#include "parson.h"

void compute_message(char *message, const char *line) {
    strcat(message, line);
    strcat(message, "\r\n");
}

int open_connection(char *host_ip, int portno, int ip_type, int socket_type, int flag) {
    struct sockaddr_in serv_addr;
    int sockfd = socket(ip_type, socket_type, flag);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = ip_type;
    serv_addr.sin_port = htons(portno);
    inet_aton(host_ip, &serv_addr.sin_addr);

    connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
    return sockfd;
}

void send_to_server(int sockfd, char *message) {
    int bytes, sent = 0;
    int total = strlen(message);

    do {
        bytes = write(sockfd, message + sent, total - sent);
        sent += bytes;
    } while (sent < total);
}

char* receive_from_server(int sockfd) {
    char response[4096];
    buffer buffer = buffer_init();
    int header_end = 0;
    int content_length = 0;

    do {
        int bytes = read(sockfd, response, 4096);
        buffer_add(&buffer, response, (size_t) bytes);
        header_end = buffer_find(&buffer, "\r\n\r\n", 4);
        if (header_end >= 0) {
            header_end += 4;
            int content_length_start = buffer_find_insensitive(&buffer, "Content-Length: ", 16);
            if (content_length_start < 0) {
                continue;           
            }
            content_length_start += 16;
            content_length = strtol(buffer.data + content_length_start, NULL, 10);
            break;
        }
    } while (1);

    size_t total = content_length + (size_t) header_end;
    while (buffer.size < total) {
        int bytes = read(sockfd, response, 4096);
        buffer_add(&buffer, response, (size_t) bytes);
    }
    buffer_add(&buffer, "", 1);
    return buffer.data;
}

// Extrage valoarea unui cookie
char* extract_cookie(char* response) {
    if (!response) return NULL;
    // Gasim pointer-ul la antetul cookie-ului
    const char *header_start_tag = "Set-Cookie: ";
    const char *header_start = strstr(response, header_start_tag);
    header_start += strlen(header_start_tag);

    // Cautam prima aparitie a lui ';', mai apoi
    // stocand valoarea cookie-ului
    const char *end = strchr(header_start, ';');
    size_t length = end - header_start;
    char *value = (char *)malloc(length + 1);

    strncpy(value, header_start, length);
    value[length] = '\0';

    return value;
}

// Returneaza un pointer catre corpul unui raspuns HTTPs
char* json_body(char* response) {
    char *body_start = strstr(response, "\r\n\r\n") + 4;
    return body_start;
}

// Gaseste si afiseaza eroarea aflata intr-un obiect JSON
void server_error(const char *json_body) {
    // Extragem valoarea din cheia "error"
    JSON_Value *root = json_parse_string(json_body);
    JSON_Object *object = json_value_get_object(root);
    const char *error = json_object_get_string(object, "error");
    printf("ERROR: %s\n", error);
}

// Verificare validitatea datelor
int valid_info(const char* s) {
    // Verificam daca sirul e gol
    if (s == NULL || s[0] == '\0') {
        return 0;
    }

    // Verificam daca contine spatii
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] == ' ') {
            return 0;
        }
    }
    return 1;
}