#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

char *compute_get_request(const char *host, const char *url, const char *query_params,
                          const char *cookie, const char *jwt_token) {
    char *message = calloc(4096, sizeof(char));
    char *line = calloc(1000, sizeof(char));

    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    if (cookie != NULL && strlen(cookie) > 0) {
        sprintf(line, "Cookie: %s", cookie);
        compute_message(message, line);
    }
    if (jwt_token != NULL && strlen(jwt_token) > 0) {
        sprintf(line, "Authorization: Bearer %s", jwt_token);
        compute_message(message, line);
    }

    compute_message(message, "");

    return message;
}

char *compute_post_request(const char *host, const char *url, const char* content_type,
    const char *body_data_json, const char *cookie, const char *jwt_token) {
    char *message = calloc(4096, sizeof(char));
    char *line = calloc(1000, sizeof(char));

    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);
    
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    int body_data_len = 0;
    if (body_data_json != NULL) {
        body_data_len = strlen(body_data_json);
    }
    sprintf(line, "Content-Length: %d", body_data_len);
    compute_message(message, line);

    if (cookie != NULL && strlen(cookie) > 0) {
        sprintf(line, "Cookie: %s", cookie);
        compute_message(message, line);
    }
    if (jwt_token != NULL && strlen(jwt_token) > 0) {
        sprintf(line, "Authorization: Bearer %s", jwt_token);
        compute_message(message, line);
    }

    compute_message(message, "");

     if (body_data_json != NULL && body_data_len > 0) {
        strcat(message, body_data_json);
    }

    return message;
}

char *compute_delete_request(const char *host, const char *url,
    const char *cookie, const char *jwt_token) {
    char *message = calloc(4096, sizeof(char));
    char *line = calloc(1000, sizeof(char));

    sprintf(line, "DELETE %s HTTP/1.1", url);
    compute_message(message, line);

    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    if (cookie != NULL && strlen(cookie) > 0) {
        sprintf(line, "Cookie: %s", cookie);
        compute_message(message, line);
    }
    if (jwt_token != NULL && strlen(jwt_token) > 0) {
        sprintf(line, "Authorization: Bearer %s", jwt_token);
        compute_message(message, line);
    }
    compute_message(message, "");

    return message;
}

char *compute_put_request(const char *host, const char *url, const char* content_type,
    const char *body_data_json, const char *cookie, const char *jwt_token) {
    char *message = calloc(4096, sizeof(char));
    char *line = calloc(1000, sizeof(char));

    sprintf(line, "PUT %s HTTP/1.1", url);
    compute_message(message, line);
    
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    int body_data_len = 0;
    if (body_data_json != NULL) {
        body_data_len = strlen(body_data_json);
    }
    sprintf(line, "Content-Length: %d", body_data_len);
    compute_message(message, line);

    if (cookie != NULL && strlen(cookie) > 0) {
        sprintf(line, "Cookie: %s", cookie);
        compute_message(message, line);
    }
    if (jwt_token != NULL && strlen(jwt_token) > 0) {
        sprintf(line, "Authorization: Bearer %s", jwt_token);
        compute_message(message, line);
    }

    compute_message(message, "");

    if (body_data_json != NULL && body_data_len > 0) {
        strcat(message, body_data_json);
    }

    return message;
}