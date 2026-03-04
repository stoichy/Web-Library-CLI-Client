#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"
#include <ctype.h>

char *IP = "63.32.125.183";
const int PORT = 8081;

char *admin_cookie = NULL;
char *user_cookie = NULL;

char *jwt_token = NULL;
char *admin_username = NULL;
char *user_username = NULL;

// Read input from the user, given a prompt
int read_input(const char *input, char *s, size_t length) {
    printf("%s=", input);
    fflush(stdout);
    fgets(s, length, stdin);
    s[strlen(s) - 1] = '\0';
    return 1;
}

void login_admin() {
    char username[50], password[50];
    int sockfd;

    JSON_Value *root = NULL;
    char *data = NULL;
    char* request = NULL;
    char* reply = NULL;

    // Read the username and password from the user
    read_input("username", username, sizeof(username));
    read_input("password", password, sizeof(password));

    // Check if we are already logged in
    if (admin_cookie != NULL) {
        return;
    }
    if (user_cookie != NULL) {
        return;
    }

    // Create a JSON object containing the client-provided data
    root = json_value_init_object();
    JSON_Object *object = json_value_get_object(root);
    json_object_set_string(object, "username", username);
    json_object_set_string(object, "password", password);
    data = json_serialize_to_string(root);

    // Open the connection, send the request, and wait for the response
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    request = compute_post_request(IP, "/api/v1/tema/admin/login", "application/json", data, NULL, NULL);
    send_to_server(sockfd, request);
    reply = receive_from_server(sockfd);
    close(sockfd);

    // If we received a positive response, extract the admin cookie
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 201 CREATED") || strstr(reply, "HTTP/1.1 204 NO CONTENT")) {
        printf("SUCCESS: Autentificare reusita\n");
        admin_cookie = extract_cookie(reply);
    } else {
        server_error(json_body(reply)); // Otherwise, print the error message received
    }
}

void login_user() {
    char admin_name[100], username[100], password[100];
    char *data = NULL;
    char *request = NULL;
    char *reply = NULL;
    int sockfd;

    // Read input from the user
    read_input("admin_username", admin_name, sizeof(admin_name));
    read_input("username", username, sizeof(username));
    read_input("password", password, sizeof(password));

    // Check if the client is already authenticated
    if (user_cookie != NULL) {
        return;
    }
    if (admin_cookie != NULL) {
        printf("ERROR: Reauthentication when already logged in\n");
        return;
    }

    // Create a JSON object with the read information
    JSON_Value* root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    json_object_set_string(root_object, "admin_username", admin_name);
    json_object_set_string(root_object, "username", username);
    json_object_set_string(root_object, "password", password);
    data = json_serialize_to_string(root_value);

    // Open the connection, send the request, and wait for the response
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    request = compute_post_request(IP, "/api/v1/tema/user/login", "application/json", data, NULL, NULL);
    send_to_server(sockfd, request);
    reply = receive_from_server(sockfd);
    close(sockfd);

    // If we received a positive response from the server, extract the user cookie
    // and set the admin and user names
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 201 CREATED") || strstr(reply, "HTTP/1.1 204 NO CONTENT")) {
        printf("SUCCESS: Login successful\n");
        user_cookie = extract_cookie(reply);
        admin_username = (char *)malloc(strlen(admin_name) + 1);
        strcpy(admin_username, admin_name);
        user_username = (char *)malloc(strlen(username) + 1);
        strcpy(user_username, username);
    } else {
        server_error(json_body(reply));
    }
}

void add_user() {
    char username[50], password[50];
    char *data = NULL;
    int sockfd;
    char *request = NULL;
    char *reply = NULL;

    // Check if we are authenticated as admin
    if (admin_cookie == NULL) {
        printf("ERROR: Missing admin role");
        return;
    }

    // Citim input-ul dat de client
    read_input("username", username, sizeof(username));
    read_input("password", password, sizeof(password));

    // Check if the provided data is in a valid format
    if (!valid_info(username) || !valid_info(password)) {
        printf("ERROR: Incomplete/incorrect information\n");
        return;
    }

    // Create a JSON object with the provided data
    JSON_Value* root = json_value_init_object();
    JSON_Object *object = json_value_get_object(root);
    json_object_set_string(object, "username", username);
    json_object_set_string(object, "password", password);
    data = json_serialize_to_string(root);

    // Send the object to the server and wait for a response
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    request = compute_post_request(IP, "/api/v1/tema/admin/users", "application/json", data, admin_cookie, NULL);
    send_to_server(sockfd, request);
    reply = receive_from_server(sockfd);
    close(sockfd);

    // If we received a positive response, just display a success message, otherwise display the received error
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 201 CREATED") || strstr(reply, "HTTP/1.1 204 NO CONTENT")) {
        printf("SUCCESS: User added successfully\n");
    } else {
        server_error(json_body(reply));
    }
}

void get_users() {
    char* request = NULL;
    char* reply = NULL;
    int sockfd;

    // Check if the client is authenticated as admin
    if (admin_cookie == NULL) {
        printf("ERROR: Missing admin permissions\n");
        return;
    }

    // Send the admin cookie to validate permissions and wait for the response
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    request = compute_get_request(IP, "/api/v1/tema/admin/users", NULL, admin_cookie, NULL);
    send_to_server(sockfd, request);
    reply = receive_from_server(sockfd);
    close(sockfd);

    // If the response is positive, extract the user data and display it
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 201 CREATED") || strstr(reply, "HTTP/1.1 204 NO CONTENT")) {
        char* body = json_body(reply);
        JSON_Value *root = json_parse_string(body);
        JSON_Object *object = json_value_get_object(root);
        JSON_Array *users = json_object_get_array(object, "users");
        printf("SUCCESS: List of users\n");
        for (size_t i = 0; i < json_array_get_count(users); i++) {
            JSON_Object *user = json_array_get_object(users, i);
            printf("#%zu %s:%s\n", i + 1, json_object_get_string(user, "username"), json_object_get_string(user, "password"));
        }
    } else {
        server_error(json_body(reply)); // Otherwise, display the received error
    }
}

void logout_admin() {
    char *request = NULL;
    char *reply = NULL;
    int sockfd = -1;

    // Check if the client is authenticated as admin
    if (admin_cookie == NULL) {
        printf("ERROR: Not authenticated\n");
        return;
    }

    // Send the admin cookie and wait for the server's response
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    request = compute_get_request(IP, "/api/v1/tema/admin/logout", NULL, admin_cookie, NULL);
    send_to_server(sockfd, request);
    reply = receive_from_server(sockfd);
    close(sockfd);

    // If the server responded positively, display a success message, otherwise display the received error
    if (reply != NULL && (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 201 CREATED") || strstr(reply, "HTTP/1.1 204 NO CONTENT"))) {
         printf("SUCCESS: Admin logged out\n");
    } else if (reply != NULL) {
        server_error(json_body(reply));
    }
    admin_cookie = NULL;
}

void logout_user() {
    char *request = NULL;
    char *reply = NULL;
    int sockfd;

    // Check if the client is authenticated as user
    if (user_cookie == NULL) {
        printf("ERROR: Not authenticated\n");
        return;
    }

    // Send the user cookie to the server and wait for the response
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    request = compute_get_request(IP, "/api/v1/tema/user/logout", NULL, user_cookie, NULL);
    send_to_server(sockfd, request);
    reply = receive_from_server(sockfd);
    close(sockfd);

    // If the server responded positively, it means the user has been logged out, so we display a success message
    if (reply != NULL && (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 201 CREATED") || strstr(reply, "HTTP/1.1 204 NO CONTENT"))) {
         printf("SUCCESS: User logged out\n");
    } else if (reply != NULL) {
        server_error(json_body(reply)); // Otherwise, display the received error
    }

    user_cookie = NULL;
}

void get_access() {
    char *request = NULL;
    char *reply = NULL;
    int sockfd = -1;

    // Check if the client is authenticated as user
    if (user_cookie == NULL) {
        printf("ERROR: Not authenticated\n");
        return;
    }

    // Send the request with the user cookie to the server and wait for the response
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    request = compute_get_request(IP, "/api/v1/tema/library/access", NULL, user_cookie, NULL);
    send_to_server(sockfd, request);
    reply = receive_from_server(sockfd);
    close(sockfd);

    // If the server responded positively, it means we received the JWT token, otherwise display the received error
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 201 CREATED") || strstr(reply, "HTTP/1.1 204 NO CONTENT")) {
        // Extract the received token and store it locally
        char *body = json_body(reply);
        JSON_Value *root = json_parse_string(body);
        JSON_Object *object = json_value_get_object(root);
        const char *token = json_object_get_string(object, "token");
        printf("SUCCESS: JWT token received\n");
        jwt_token = (char *)malloc(strlen(token) + 1);
        strcpy(jwt_token, token);
    } else {
        server_error(json_body(reply));
    }
}

void get_movies() {
    char *request = NULL;
    char *reply = NULL;
    int sockfd;

    // Check if the client has the JWT token
    if (jwt_token == NULL) {
        printf("ERROR: No library access\n");
        return;
    }

    // Send the request with the user cookie and JWT token
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    request = compute_get_request(IP, "/api/v1/tema/library/movies", NULL, user_cookie, jwt_token);
    send_to_server(sockfd, request);
    reply = receive_from_server(sockfd);
    close(sockfd);

    // If we received a positive response from the server, 
    // it means we have access to the library, so we extract 
    // the received data and display a success message,
    // otherwise we display the received error
    if (strstr(reply, "HTTP/1.1 200 OK")) {
        char *body = json_body(reply);
        JSON_Value *root = json_parse_string(body);
        JSON_Object *object = json_value_get_object(root);
        JSON_Array *movies = json_object_get_array(object, "movies");
        printf("SUCCESS: List of movies\n");
        size_t count = json_array_get_count(movies);
        // For each movie, extract the title and id and display them
        for (size_t i = 0; i < count; i++) {
            JSON_Object *movie = json_array_get_object(movies, i);
            const char *title = json_object_get_string(movie, "title");
            long id = json_object_get_number(movie, "id");
            printf("#%ld %s\n", id, title);
        }
    } else {
        server_error(json_body(reply));
    }
}

void add_movie() {
    char title[100], year[100], description[100], rating[100];
    char *data = NULL;
    char *request = NULL;
    char *reply = NULL;
    int sockfd;

    // Check if the client has the JWT token
    if (jwt_token == NULL) {
        printf("ERROR: No library access\n");
        return;
    }

    // Read input from the user
    read_input("title", title, sizeof(title));
    read_input("year", year, sizeof(year));
    read_input("description", description, sizeof(description));
    read_input("rating", rating, sizeof(rating));

    // Verify the validity of the received data
    for (int i = 0; year[i] != '\0'; i++) {
        if (!isdigit((unsigned char)year[i])) {
            printf("ERROR: Year contains invalid characters\n");
            return;
        }
    }

    if (description[0] == '\0') {
        printf("ERROR: Description is empty\n");
        return;
    }

    char *rating_pointer;
    double rating_val = strtod(rating, &rating_pointer);
    if (rating_pointer == rating || *rating_pointer!= '\0') {
        printf("ERROR: Rating is not a number\n");
        return;
    }
    if (rating_val < 0.0 || rating_val > 10.0) {
        printf("ERROR: Rating must be between 0.0 and 10.0.\n");
        return;
    }
    
    // Initialize a JSON object with the received information
    JSON_Value* root = json_value_init_object();
    JSON_Object *object = json_value_get_object(root);
    json_object_set_string(object, "title", title);
    json_object_set_number(object, "year", atoi(year));
    json_object_set_string(object, "description", description);
    json_object_set_number(object, "rating", atof(rating));
    data = json_serialize_to_string(root);

    // Transmit the object to the server and wait for a response
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    request = compute_post_request(IP, "/api/v1/tema/library/movies",
                                   "application/json", data, user_cookie, jwt_token);
    send_to_server(sockfd, request);
    reply = receive_from_server(sockfd);
    close(sockfd);

    // If we received a positive response, display a success message, otherwise display the received error
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 201 CREATED") || strstr(reply, "HTTP/1.1 204 NO CONTENT")) {
        printf("SUCCESS: Movie added\n");
    } else {
        server_error(json_body(reply));
    }
}

void get_movie() {
    char id[50];
    char path[100];
    int sockfd;

    // Check if the client has the JWT token
    if (jwt_token == NULL) {
        printf("ERROR: No library access\n");
        return;
    }

    // Read input from the user and create the corresponding path
    read_input("id", id, sizeof(id));
    snprintf(path, sizeof(path), "/api/v1/tema/library/movies/%s", id);

    // Transmit the corresponding request to the server and wait for a response
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    char* request = compute_get_request(IP, path, NULL, user_cookie, jwt_token);
    send_to_server(sockfd, request);
    char* reply = receive_from_server(sockfd);
    close(sockfd);

    // If we received a positive response, display the movie details and a success message
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 201 CREATED") || strstr(reply, "HTTP/1.1 204 NO CONTENT")) {
        char *body = json_body(reply);
        JSON_Value *root = json_parse_string(body);
        JSON_Object *movie = json_value_get_object(root);
        printf("SUCCESS: Movie details\n");
        const char* title = json_object_get_string(movie, "title");
        printf("title: %s\n", title);
        double year = json_object_get_number(movie, "year");
        printf("year: %.0f\n", year);
        const char* description = json_object_get_string(movie, "description");
        printf("description: %s\n", description);
        const char *rating = json_object_get_string(movie, "rating");
        printf("rating: %.1f\n", atof(rating));
    } else {
        server_error(json_body(reply)); // Otherwise, display the received error
    }
}

void update_movie() {
    char id[100], title[1000], year[1000], description[1000], rating[1000];
    char *data = NULL;
    char path[1000];
    int sockfd;

    // Check if the client has the JWT token
    if (jwt_token == NULL) {
        printf("ERROR: No library access\n");
        return;
    }

    // Read input from the user
    read_input("id", id, sizeof(id));
    read_input("title", title, sizeof(title));
    read_input("year", year, sizeof(year));
    read_input("description", description, sizeof(description));
    read_input("rating", rating, sizeof(rating));

    // Verify the validity of the data
    for (int i = 0; year[i] != '\0'; i++) {
        if (!isdigit((unsigned char)year[i])) {
            printf("ERROR: Year contains invalid characters\n");
            return;
        }
    }

    if (description[0] == '\0') {
        printf("ERROR: Description is empty\n");
        return;
    }

    char *rating_pointer;
    double rating_float = strtod(rating, &rating_pointer);
    if (rating_pointer == rating || *rating_pointer != '\0') {
        printf("ERROR: Rating is not a valid number\n");
        return;
    }
    if (rating_float < 0.0 || rating_float > 10.0) {
        printf("ERROR: Rating must be between 0.0 and 10.0\n");
        return;
    }

    // Initialize a JSON object with the received information
    JSON_Value* root = json_value_init_object();
    JSON_Object* object = json_value_get_object(root);
    json_object_set_string(object, "title", title);
    json_object_set_number(object, "year", atoi(year));
    json_object_set_string(object, "description", description);
    json_object_set_number(object, "rating", atof(rating));
    data = json_serialize_to_string(root);

    // Initialize the corresponding path
    snprintf(path, sizeof(path), "/api/v1/tema/library/movies/%s", id);

    // Send the object to the server and wait for a response
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    char* request = compute_put_request(IP, path, "application/json", data, user_cookie, jwt_token);
    send_to_server(sockfd, request);
    char* reply = receive_from_server(sockfd);
    close(sockfd);

    // If the response was positive, display a success message
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 204 No Content")) {
        printf("SUCCESS: Movie updated\n");
    } else {
        server_error(json_body(reply)); // Otherwise, display the received error
    }
}

void delete_movie() {
    char id[50];
    char path[1000];
    int sockfd;

    // Check if the client has the JWT token
    if (jwt_token == NULL) {
        printf("ERROR: No library access\n");
        return;
    }
    
    // Read input from the user and set the path for the server
    read_input("id", id, sizeof(id));
    snprintf(path, sizeof(path), "/api/v1/tema/library/movies/%s", id);

    // Send the request and wait for a response
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    char* request = compute_delete_request(IP, path, user_cookie, jwt_token);
    send_to_server(sockfd, request);
    char* reply = receive_from_server(sockfd);
    close(sockfd);

    // If the response was positive, display a success message
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 204 No Content")) {
        printf("SUCCESS: Movie deleted successfully\n");
    } else {
        server_error(json_body(reply)); // Otherwise, display the received error
    }
}

void add_collection() {
    char title[1000];
    char num_movies_string[10];
    int num_movies;

    JSON_Value *collection = NULL;
    char *collection_data = NULL;
    int sockfd;
    long collection_id = -1;
    int operations_successful = 1;

    // Check if the client has the JWT token
    if (jwt_token == NULL) {
        printf("ERROR: No library access\n");
        return;
    }

    // Read input from the user
    read_input("title", title, sizeof(title));
    read_input("num_movies", num_movies_string, sizeof(num_movies_string));
    num_movies = atoi(num_movies_string);

    // Check the validity of the input
    if (title[0] == '\0') {
        printf("ERROR: Title cannot be empty\n");
        return;
    }

    // Read the movie IDs from the user
    long* movie_ids = (long *)malloc(num_movies * sizeof(long));
    for (int i = 0; i < num_movies; ++i) {
        char movie_id_prompt[30];
        char movie_id[50];
        snprintf(movie_id_prompt, sizeof(movie_id_prompt), "movie_id[%d]", i);
        read_input(movie_id_prompt, movie_id, sizeof(movie_id));
        movie_ids[i] = atol(movie_id);
    }

    // Initialize a JSON object with the collection data provided as input
    collection = json_value_init_object();
    json_object_set_string(json_value_get_object(collection), "title", title);
    collection_data = json_serialize_to_string(collection);

    // Send a request with the JSON object and wait for a response
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    char* request = compute_post_request(IP, "/api/v1/tema/library/collections", "application/json",
                                     collection_data, user_cookie, jwt_token);
    send_to_server(sockfd, request);
    char* reply = receive_from_server(sockfd);
    close(sockfd);

    // If the response was positive, it means the collection was created successfully
    if (strstr(reply, "HTTP/1.1 201 CREATED") || strstr(reply, "HTTP/1.1 200 OK")) {
        char *collection_body = json_body(reply);
        JSON_Value *collection_json = json_parse_string(collection_body);
        JSON_Object* collection_object = json_value_get_object(collection_json);
        collection_id = (long)json_object_get_number(collection_object, "id");
    } else {
        server_error(json_body(reply)); // Otherwise, display the received error
    }
    
    // Thus, for each movie we want to add, we send a request with its data
    // to the server
    if (operations_successful && num_movies > 0 && collection_id >= 0) {
        for (int i = 0; i < num_movies; ++i) {
            // Create a JSON object with the data of a single movie
            JSON_Value *movie_root = json_value_init_object();
            JSON_Object *movie = json_value_get_object(movie_root);
            json_object_set_number(movie, "id", movie_ids[i]);
            char *movie_data = json_serialize_to_string(movie_root);
            char movie_path[1000];
            snprintf(movie_path, sizeof(movie_path), "/api/v1/tema/library/collections/%ld/movies", collection_id);

            // Send the request to add the movie and wait for a response
            sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
            request = compute_post_request(IP, movie_path, "application/json",
                                             movie_data, user_cookie, jwt_token);
            send_to_server(sockfd, request);
            reply = receive_from_server(sockfd);
            close(sockfd);

            // If we did not receive a positive response for adding a movie, it means we failed to add the collection
            if (!(reply != NULL && (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 201 CREATED")))) {
                operations_successful = 0; 
                break; 
            }
        }
    }

    if (operations_successful) {
        printf("SUCCESS: Collection added\n");
    } else {
        printf("ERROR: The operation of adding movies was not completed\n"); 
    }
}

void get_collections() {
    char *request = NULL;
    char *reply = NULL;
    int sockfd;

    // Check if the client has a JWT token
    if (jwt_token == NULL) {
        printf("ERROR: No library access\n");
        return;
    }

    // Send the request to the server and wait for a response
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    request = compute_get_request(IP, "/api/v1/tema/library/collections", NULL,
                                    user_cookie, jwt_token);
    send_to_server(sockfd, request);
    reply = receive_from_server(sockfd);
    close(sockfd);

    // If we received a positive response, display the details of the received collections
    if (strstr(reply, "HTTP/1.1 200 OK")) {
        printf("SUCCESS: List of collections\n");
        char *body = json_body(reply);
        JSON_Value *root = json_parse_string(body);
        JSON_Object *object = json_value_get_object(root);
        JSON_Array* collections = json_object_get_array(object, "collections");
        size_t count = json_array_get_count(collections);
        for (size_t i = 0; i < count; i++) {
            JSON_Object *collection = json_array_get_object(collections, i);
            long collection_id = json_object_get_number(collection, "id");
            printf("#%ld: %s\n", collection_id, json_object_get_string(collection, "title"));
        }
    } else {
        server_error(json_body(reply)); // Otherwise, display the received error
    }

}

void get_collection() {
    char id[50];
    char path[1000];
    int sockfd = -1;

    // Check if the client has a JWT token
    if (jwt_token == NULL) {
        printf("ERROR: Fara acces library\n");
        return;
    }

    // Read input from the user and set the corresponding path
    read_input("id", id, sizeof(id));
    snprintf(path, sizeof(path), "/api/v1/tema/library/collections/%s", id);

    // Send the request to the server and wait for a response
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    char* request = compute_get_request(IP, path, NULL, user_cookie, jwt_token);
    send_to_server(sockfd, request);
    char* reply = receive_from_server(sockfd);
    close(sockfd);

    // If we received a positive response, display the details of the collection
    if (strstr(reply, "HTTP/1.1 200 OK")) {
        JSON_Value *root = json_parse_string(json_body(reply));
        JSON_Object *collection = json_value_get_object(root);
        printf("SUCCESS: Collection details\n");
        const char *title = json_object_get_string(collection, "title");
        printf("title: %s\n", title);
        const char *owner = json_object_get_string(collection, "owner");
        printf("owner: %s\n", owner);
        JSON_Array *movies = json_object_get_array(collection, "movies");
        size_t movies_count = json_array_get_count(movies);

        // Display the details of each movie in the collection
        for (size_t i = 0; i < movies_count; i++) {
            JSON_Object *movie_data = json_array_get_object(movies, i);
            long movie_id = (long)json_object_get_number(movie_data, "id");
            const char *movie_title = json_object_get_string(movie_data, "title");
            printf("#%ld: %s\n", movie_id, movie_title);
        }
    } else {
        server_error(json_body(reply)); // Otherwise, display the received error
    }

}

void delete_collection() {
    char id[50];
    char path[1000];
    int sockfd;

    // Check if the client has a JWT token
    if (jwt_token == NULL) {
        printf("ERROR: No library access\n");
        return;
    }

    // Read input from the user and set the corresponding path
    read_input("id", id, sizeof(id));
    snprintf(path, sizeof(path), "/api/v1/tema/library/collections/%s", id);

    // Send the request to the server and wait for a response
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    char* request = compute_delete_request(IP, path, user_cookie, jwt_token);
    send_to_server(sockfd, request);
    char* reply = receive_from_server(sockfd);
    close(sockfd);

    // If we received a positive response, display a success message,
    // otherwise display the received error
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 204 No Content")) {
        printf("SUCCESS: Collection deleted\n");
    } else {
        server_error(json_body(reply));
    }
}

void add_movie_to_collection() {
    char collection_id[50];
    char movie_id[50];
    char path[1000];

    JSON_Value *root = NULL;
    JSON_Object *object = NULL;
    char *data = NULL;

    int sockfd = -1;

    // Check if the client has a JWT token
    if (jwt_token == NULL) {
        printf("ERROR: No library access\n");
        return;
    }

    // Read input from the user
    read_input("collection_id", collection_id, sizeof(collection_id));
    read_input("movie_id", movie_id, sizeof(movie_id));

    // Check the validity of the data
    if (collection_id[0] == '\0' || movie_id[0] == '\0') {
        printf("ERROR: Invalid/incomplete data.\n");
        return;
    }
    
    // Store the corresponding path from the server
    snprintf(path, sizeof(path), "/api/v1/tema/library/collections/%s/movies", collection_id);

    // Initialize a JSON object for sending data
    root = json_value_init_object();
    object = json_value_get_object(root);
    json_object_set_number(object, "id", atol(movie_id));
    data = json_serialize_to_string(root);

    // Send the request to the server and wait for a response
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    char* request = compute_post_request(IP, path, "application/json", data, user_cookie, jwt_token);
    send_to_server(sockfd, request);
    char* reply = receive_from_server(sockfd);
    close(sockfd);

    // If we received a positive response, display a success message,
    // otherwise display the received error
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 201 CREATED") || strstr(reply, "HTTP/1.1 204 NO CONTENT")) {
        printf("SUCCESS: Movie added to collection\n");
    } else {
        server_error(json_body(reply));
    }
}

void delete_movie_from_collection() {
    char collection_id[50];
    char movie_id[50];
    char path[1000];

    int sockfd = -1;

    // Check if the client has a JWT token
    if (jwt_token == NULL) {
        printf("ERROR: No library access\n");
        return;
    }

    // Read input from the user and set the corresponding path
    read_input("collection_id", collection_id, sizeof(collection_id));
    read_input("movie_id", movie_id, sizeof(movie_id));
    snprintf(path, sizeof(path), "/api/v1/tema/library/collections/%s/movies/%s", collection_id, movie_id);

    // Send the request to the server and wait for a response
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    char* request = compute_delete_request(IP, path, user_cookie, jwt_token);
    send_to_server(sockfd, request);
    char* reply = receive_from_server(sockfd);
    close(sockfd);
    
    // If we received a positive response, display a success message,
    // otherwise display the received error
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 204 No Content")) {
        printf("SUCCESS: Movie deleted from collection\n");
    } else {
        server_error(json_body(reply));
    }
}

void delete_user() {
    char user[100];
    char path[1000];
    char *request = NULL;
    char *reply = NULL;
    int sockfd;

    // Check if the client is authenticated as admin
    if (admin_cookie == NULL) {
        printf("ERROR: Missing admin role\n");
        return;
    }

    // Read input from the user and set the corresponding path
    read_input("username", user, sizeof(user));
    snprintf(path, sizeof(path), "/api/v1/tema/admin/users/%s", user);

    // Check the validity of the data
    if (user[0] == '\0') {
        printf("ERROR: Username cannot be empty.\n");
        return;
    }

    // Send the request to the server and wait for a response
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    request = compute_delete_request(IP, path, admin_cookie, NULL); 
    send_to_server(sockfd, request);
    reply = receive_from_server(sockfd);
    close(sockfd);

    // If we received a positive response, display a success message,
    // otherwise display the received error
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 204 No Content")) {
        printf("SUCCESS: User deleted\n");
    } else {
        server_error(json_body(reply));
    }
}

int main() {
    char command[50];

    while(1) {
        // Read a command from the user
        fgets(command, sizeof(command), stdin);
        command[strlen(command) - 1] = '\0';

        // Check the type of command received and call
        // the corresponding function
        if (strcmp(command, "login_admin") == 0) login_admin();
        if (strcmp(command, "add_user") == 0) add_user();
        if (strcmp(command, "add_movie") == 0) add_movie();
        if (strcmp(command, "get_users") == 0) get_users();
        if (strcmp(command, "logout_admin") == 0) logout_admin();
        if (strcmp(command, "login") == 0) login_user();
        if (strcmp(command, "get_access") == 0) get_access();
        if (strcmp(command, "get_movies") == 0) get_movies();
        if (strcmp(command, "logout") == 0) logout_user();
        if (strcmp(command, "get_movie") == 0) get_movie();
        if (strcmp(command, "update_movie") == 0) update_movie();
        if (strcmp(command, "delete_movie") == 0) delete_movie();
        if (strcmp(command, "add_collection") == 0) add_collection();
        if (strcmp(command, "get_collections") == 0) get_collections();
        if (strcmp(command, "get_collection") == 0) get_collection();
        if (strcmp(command, "delete_collection") == 0) delete_collection();
        if (strcmp(command, "add_movie_to_collection") == 0) add_movie_to_collection();
        if (strcmp(command, "delete_movie_from_collection") == 0) delete_movie_from_collection();
        if (strcmp(command, "delete_user") == 0) delete_user();
        if (strcmp(command, "exit") == 0) break;
    }
    return 0;
}