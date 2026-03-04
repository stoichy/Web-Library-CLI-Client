#ifndef _HELPERS_
#define _HELPERS_

void compute_message(char *message, const char *line);

int open_connection(char *host_ip, int portno, int ip_type, int socket_type, int flag);

void send_to_server(int sockfd, char *message);

char *receive_from_server(int sockfd);

char* extract_cookie(char* response);

char* json_body(char* response);

void server_error(const char *json_body);

int valid_info(const char* str);

#endif
