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

// Citim input-ul dat de client la tastatura
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

    // Citim inputul
    read_input("username", username, sizeof(username));
    read_input("password", password, sizeof(password));

    // Verificam daca suntem deja logati
    if (admin_cookie != NULL) {
        return;
    }
    if (user_cookie != NULL) {
        return;
    }

    // Cream un obiect JSON ce contine datele date de client
    root = json_value_init_object();
    JSON_Object *object = json_value_get_object(root);
    json_object_set_string(object, "username", username);
    json_object_set_string(object, "password", password);
    data = json_serialize_to_string(root);

    // Deschidem conexiunea, trimitem cererea  si asteptam sa primim raspunsul
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    request = compute_post_request(IP, "/api/v1/tema/admin/login", "application/json", data, NULL, NULL);
    send_to_server(sockfd, request);
    reply = receive_from_server(sockfd);
    close(sockfd);

    // Daca am primit un raspuns pozitiv, extragem cookie-ul corespunzator adminului
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 201 CREATED") || strstr(reply, "HTTP/1.1 204 NO CONTENT")) {
        printf("SUCCESS: Autentificare reusita\n");
        admin_cookie = extract_cookie(reply);
    } else {
        server_error(json_body(reply)); // Altfel afisam eroarea primita de la server
    }
}

void login_user() {
    char admin_name[100], username[100], password[100];
    char *data = NULL;
    char *request = NULL;
    char *reply = NULL;
    int sockfd;

    // Citim input-ul utilizatorului de la tastatura
    read_input("admin_username", admin_name, sizeof(admin_name));
    read_input("username", username, sizeof(username));
    read_input("password", password, sizeof(password));

    // Verificam daca clientul este deja autentificat
    if (user_cookie != NULL) {
        return;
    }
    if (admin_cookie != NULL) {
        printf("ERROR:  Reautentificare când deja logat\n");
        return;
    }

    // Cream un obiect JSON cu informatiile citite
    JSON_Value* root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    json_object_set_string(root_object, "admin_username", admin_name);
    json_object_set_string(root_object, "username", username);
    json_object_set_string(root_object, "password", password);
    data = json_serialize_to_string(root_value);

    // Deschidem conexiunea cu server-ul, trimitem cererea corespunzatoare si asteptam raspunsul
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    request = compute_post_request(IP, "/api/v1/tema/user/login", "application/json", data, NULL, NULL);
    send_to_server(sockfd, request);
    reply = receive_from_server(sockfd);
    close(sockfd);

    // Daca primim un raspunz pozitiv de la server, extragem cookie-ul utilizatorului
    // si setam numele adminului si al utilizatorului
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 201 CREATED") || strstr(reply, "HTTP/1.1 204 NO CONTENT")) {
        printf("SUCCESS: Autentificare reusita\n");
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

    // Verificam daca suntem autentificati ca admin
    if (admin_cookie == NULL) {
        printf("ERROR: Lipsa rol admin");
        return;
    }

    // Citim input-ul dat de client
    read_input("username", username, sizeof(username));
    read_input("password", password, sizeof(password));

    // Verificam daca datele oferite de client
    // sunt intr-un format corespunzator
    if (!valid_info(username) || !valid_info(password)) {
        printf("ERROR: Informatii incomplete/incorecte\n");
        return;
    }

    // Cream un obiect JSON cu datele date de client
    JSON_Value* root = json_value_init_object();
    JSON_Object *object = json_value_get_object(root);
    json_object_set_string(object, "username", username);
    json_object_set_string(object, "password", password);
    data = json_serialize_to_string(root);

    // Trimitem obiectul catre server si asteptam un raspuns
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    request = compute_post_request(IP, "/api/v1/tema/admin/users", "application/json", data, admin_cookie, NULL);
    send_to_server(sockfd, request);
    reply = receive_from_server(sockfd);
    close(sockfd);

    // Daca primim un raspuns pozitiv, doar afisam un mesaj de succes, altfel afisam eroarea primita
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 201 CREATED") || strstr(reply, "HTTP/1.1 204 NO CONTENT")) {
        printf("SUCCESS: Utilizator adăugat cu succes\n");
    } else {
        server_error(json_body(reply));
    }
}

void get_users() {
    char* request = NULL;
    char* reply = NULL;
    int sockfd;

    // Verificam daca clientul este autentificat ca admin
    if (admin_cookie == NULL) {
        printf("ERROR: Lipsa permisiuni admin\n");
        return;
    }

    // Trimitem cookie-ul adminului pentru a ni se valida permisiunea si asteptam raspunsul
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    request = compute_get_request(IP, "/api/v1/tema/admin/users", NULL, admin_cookie, NULL);
    send_to_server(sockfd, request);
    reply = receive_from_server(sockfd);
    close(sockfd);

    // Daca raspunsul e pozitiv, extragem datele utilizatorilor si le afisam
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 201 CREATED") || strstr(reply, "HTTP/1.1 204 NO CONTENT")) {
        char* body = json_body(reply);
        JSON_Value *root = json_parse_string(body);
        JSON_Object *object = json_value_get_object(root);
        JSON_Array *users = json_object_get_array(object, "users");
        printf("SUCCESS: Lista utilizatorilor\n");
        for (size_t i = 0; i < json_array_get_count(users); i++) {
            JSON_Object *user = json_array_get_object(users, i);
            printf("#%zu %s:%s\n", i + 1, json_object_get_string(user, "username"), json_object_get_string(user, "password"));
        }
    } else {
        server_error(json_body(reply)); // Altfel, afisam eroarea primita
    }
}

void logout_admin() {
    char *request = NULL;
    char *reply = NULL;
    int sockfd = -1;

    // Verificam daca clientul este autentificat ca admin
    if (admin_cookie == NULL) {
        printf("ERROR: Neautentificat\n");
        return;
    }

    // Trimitem cookie-ul adminului si asteptam raspunsul server-ului
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    request = compute_get_request(IP, "/api/v1/tema/admin/logout", NULL, admin_cookie, NULL);
    send_to_server(sockfd, request);
    reply = receive_from_server(sockfd);
    close(sockfd);

    // Daca server-ul a raspuns pozitiv, afisam un mesaj de succes, altfel afisam mesajul cu eroarea primita
    if (reply != NULL && (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 201 CREATED") || strstr(reply, "HTTP/1.1 204 NO CONTENT"))) {
         printf("SUCCESS: Admin delogat\n");
    } else if (reply != NULL) {
        server_error(json_body(reply));
    }
    admin_cookie = NULL;
}

void logout_user() {
    char *request = NULL;
    char *reply = NULL;
    int sockfd;

    // Verificam daca clientul este autentificat ca user
    if (user_cookie == NULL) {
        printf("ERROR: Neautentificat\n");
        return;
    }

    // Transmitem mesajul cu cookie-ul user-ului la server si asteptam raspuns
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    request = compute_get_request(IP, "/api/v1/tema/user/logout", NULL, user_cookie, NULL);
    send_to_server(sockfd, request);
    reply = receive_from_server(sockfd);
    close(sockfd);

    // Daca server-ul a raspuns pozitiv, inseamna ca s-a delogat clientul si afisam un mesaj de succes
    if (reply != NULL && (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 201 CREATED") || strstr(reply, "HTTP/1.1 204 NO CONTENT"))) {
         printf("SUCCESS: Utilizator delogat\n");
    } else if (reply != NULL) {
        server_error(json_body(reply)); // Altfel, afisam eroarea primita
    }

    user_cookie = NULL;
}

void get_access() {
    char *request = NULL;
    char *reply = NULL;
    int sockfd = -1;

    // Verificam daca clientul este autentificat
    // ca utilizator  
    if (user_cookie == NULL) {
        printf("ERROR: Neautentificat\n");
        return;
    }

    // Transmitem cererea insotita de user cookie server-ului si asteptam raspuns
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    request = compute_get_request(IP, "/api/v1/tema/library/access", NULL, user_cookie, NULL);
    send_to_server(sockfd, request);
    reply = receive_from_server(sockfd);
    close(sockfd);

    // Daca server-ul a raspuns pozitiv, inseamna ca am primit token-ul jwt, altfel afisam eroarea primita
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 201 CREATED") || strstr(reply, "HTTP/1.1 204 NO CONTENT")) {
        // Extragem token-ul primit si il stocam local
        char *body = json_body(reply);
        JSON_Value *root = json_parse_string(body);
        JSON_Object *object = json_value_get_object(root);
        const char *token = json_object_get_string(object, "token");
        printf("SUCCESS: Token JWT primit\n");
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

    // Verificam daca clientul are token-ul JWT
    if (jwt_token == NULL) {
        printf("ERROR: Fara acces library\n");
        return;
    }

    // Trimitem cererea cu cookie-ul user-ului si token-ul JWT
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    request = compute_get_request(IP, "/api/v1/tema/library/movies", NULL, user_cookie, jwt_token);
    send_to_server(sockfd, request);
    reply = receive_from_server(sockfd);
    close(sockfd);

    // Daca am primit un raspuns pozitiv,
    // extragem raspunsul si afisam un mesaj de succes
    if (strstr(reply, "HTTP/1.1 200 OK")) {
        char *body = json_body(reply);
        JSON_Value *root = json_parse_string(body);
        JSON_Object *object = json_value_get_object(root);
        JSON_Array *movies = json_object_get_array(object, "movies");
        printf("SUCCESS: Lista filmelor\n");
        size_t count = json_array_get_count(movies);
        // Pentru fiecare film, extragem titlul si id-ul si le afisam
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

    // Verificam daca clientul are token-ul JWT
    if (jwt_token == NULL) {
        printf("ERROR: Fara acces library\n");
        return;
    }

    // Citim de la tastatura input-ul clientului
    read_input("title", title, sizeof(title));
    read_input("year", year, sizeof(year));
    read_input("description", description, sizeof(description));
    read_input("rating", rating, sizeof(rating));

    // Verificam validitatea datelor primite
    for (int i = 0; year[i] != '\0'; i++) {
        if (!isdigit((unsigned char)year[i])) {
            printf("ERROR: Anul contine caractere interzise\n");
            return;
        }
    }

    if (description[0] == '\0') {
        printf("ERROR: Descrierea este goala\n");
        return;
    }

    char *rating_pointer;
    double rating_val = strtod(rating, &rating_pointer);
    if (rating_pointer == rating || *rating_pointer!= '\0') {
        printf("ERROR: Rating-ul nu este un numar\n");
        return;
    }
    if (rating_val < 0.0 || rating_val > 10.0) {
        printf("ERROR: Rating-ul trebuie sa fie intre 0.0 si 10.0.\n");
        return;
    }
    
    // Initializam un obiect JSON cu informatiile primite
    JSON_Value* root = json_value_init_object();
    JSON_Object *object = json_value_get_object(root);
    json_object_set_string(object, "title", title);
    json_object_set_number(object, "year", atoi(year));
    json_object_set_string(object, "description", description);
    json_object_set_number(object, "rating", atof(rating));
    data = json_serialize_to_string(root);

    // Transmitem obiectul la server si asteptam un raspuns
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    request = compute_post_request(IP, "/api/v1/tema/library/movies",
                                   "application/json", data, user_cookie, jwt_token);
    send_to_server(sockfd, request);
    reply = receive_from_server(sockfd);
    close(sockfd);

    // Daca am primit un raspuns pozitiv, afisam mesaj de succes, altfel afisam eroarea primita
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 201 CREATED") || strstr(reply, "HTTP/1.1 204 NO CONTENT")) {
        printf("SUCCESS: Film adăugat\n");
    } else {
        server_error(json_body(reply));
    }
}

void get_movie() {
    char id[50];
    char path[100];
    int sockfd;

    // Verificam daca clientul are token-ul JWT
    if (jwt_token == NULL) {
        printf("ERROR: Fara acces library\n");
        return;
    }

    // Citim input-ul si creeam calea corespunzatoare
    read_input("id", id, sizeof(id));
    snprintf(path, sizeof(path), "/api/v1/tema/library/movies/%s", id);

    // Transmitem cererea corespunzatoare catre server si asteptam raspuns
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    char* request = compute_get_request(IP, path, NULL, user_cookie, jwt_token);
    send_to_server(sockfd, request);
    char* reply = receive_from_server(sockfd);
    close(sockfd);

    // Daca am primit un raspuns afirmativ, afisam detaliile filmului si un mesaj de succes
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 201 CREATED") || strstr(reply, "HTTP/1.1 204 NO CONTENT")) {
        char *body = json_body(reply);
        JSON_Value *root = json_parse_string(body);
        JSON_Object *movie = json_value_get_object(root);
        printf("SUCCESS: Detalii film\n");
        const char* title = json_object_get_string(movie, "title");
        printf("title: %s\n", title);
        double year = json_object_get_number(movie, "year");
        printf("year: %.0f\n", year);
        const char* description = json_object_get_string(movie, "description");
        printf("description: %s\n", description);
        const char *rating = json_object_get_string(movie, "rating");
        printf("rating: %.1f\n", atof(rating));
    } else {
        server_error(json_body(reply)); // Altfel, afisam eroarea primita
    }
}

void update_movie() {
    char id[100], title[1000], year[1000], description[1000], rating[1000];
    char *data = NULL;
    char path[1000];
    int sockfd;

    // Verificam daca clientul are token-ul JWT
    if (jwt_token == NULL) {
        printf("ERROR: Fara acces library\n");
        return;
    }

    // Citim input-ului client-ului de la tastatura
    read_input("id", id, sizeof(id));
    read_input("title", title, sizeof(title));
    read_input("year", year, sizeof(year));
    read_input("description", description, sizeof(description));
    read_input("rating", rating, sizeof(rating));

    // Verificam validitatea datelor
    for (int i = 0; year[i] != '\0'; i++) {
        if (!isdigit((unsigned char)year[i])) {
            printf("ERROR: Anul contine caractere interzise\n");
            return;
        }
    }

    if (description[0] == '\0') {
        printf("ERROR: Descrierea este goala\n");
        return;
    }

    char *rating_pointer;
    double rating_float = strtod(rating, &rating_pointer);
    if (rating_pointer == rating || *rating_pointer != '\0') {
        printf("ERROR: Rating-ul nu este un numar valid\n");
        return;
    }
    if (rating_float < 0.0 || rating_float > 10.0) {
        printf("ERROR: Rating-ul trebuie sa fie intre 0.0 si 10.0\n");
        return;
    }

    // Initializam un obiect JSON cu informatiile primite
    JSON_Value* root = json_value_init_object();
    JSON_Object* object = json_value_get_object(root);
    json_object_set_string(object, "title", title);
    json_object_set_number(object, "year", atoi(year));
    json_object_set_string(object, "description", description);
    json_object_set_number(object, "rating", atof(rating));
    data = json_serialize_to_string(root);

    // Initializam calea corespunzatoare
    snprintf(path, sizeof(path), "/api/v1/tema/library/movies/%s", id);

    // Trimitem obiectul catre server si asteptam un raspuns
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    char* request = compute_put_request(IP, path, "application/json", data, user_cookie, jwt_token);
    send_to_server(sockfd, request);
    char* reply = receive_from_server(sockfd);
    close(sockfd);

    // Daca raspunsul a fost pozitiv, atunci afisam un mesaj de succes
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 204 No Content")) {
        printf("SUCCESS: Film actualizat\n");
    } else {
        server_error(json_body(reply)); // Altfel, afisam eroarea primita
    }
}

void delete_movie() {
    char id[50];
    char path[1000];
    int sockfd;

    // Verificam daca client-ul are token-ul JWT
    if (jwt_token == NULL) {
        printf("ERROR: Fara acces library\n");
        return;
    }
    
    // Citim datele date de client de la tastatura si stabilim calea
    // pentru server
    read_input("id", id, sizeof(id));
    snprintf(path, sizeof(path), "/api/v1/tema/library/movies/%s", id);

    // Trimitem cererea si asteptam raspuns
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    char* request = compute_delete_request(IP, path, user_cookie, jwt_token);
    send_to_server(sockfd, request);
    char* reply = receive_from_server(sockfd);
    close(sockfd);

    // Daca am primit un mesaj pozitiv atunci afisam un mesaj de succes
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 204 No Content")) {
        printf("SUCCESS: Film sters cu succes\n");
    } else {
        server_error(json_body(reply)); // Altfel afisam eroarea primita
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

    // Verificam daca clientul are token-ul JWT
    if (jwt_token == NULL) {
        printf("ERROR: Fara acces library\n");
        return;
    }

    // Citim input-ul de la tastatura al clientului
    read_input("title", title, sizeof(title));
    read_input("num_movies", num_movies_string, sizeof(num_movies_string));
    num_movies = atoi(num_movies_string);

    // Verificam validitatea input-ului
    if (title[0] == '\0') {
        printf("ERROR: Titlul nu poate fi gol\n");
        return;
    }

    // Citim de la tastatura id-urile filmelor
    long* movie_ids = (long *)malloc(num_movies * sizeof(long));
    for (int i = 0; i < num_movies; ++i) {
        char movie_id_prompt[30];
        char movie_id[50];
        snprintf(movie_id_prompt, sizeof(movie_id_prompt), "movie_id[%d]", i);
        read_input(movie_id_prompt, movie_id, sizeof(movie_id));
        movie_ids[i] = atol(movie_id);
    }

    // Initializam un obiect JSON cu datele colectiei date ca input
    collection = json_value_init_object();
    json_object_set_string(json_value_get_object(collection), "title", title);
    collection_data = json_serialize_to_string(collection);

    // Trimitem o cerere cu obiectul JSON si asteptam raspuns
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    char* request = compute_post_request(IP, "/api/v1/tema/library/collections", "application/json",
                                     collection_data, user_cookie, jwt_token);
    send_to_server(sockfd, request);
    char* reply = receive_from_server(sockfd);
    close(sockfd);

    // Daca am primit un mesaj pozitiv, inseamna ca am creat colectia cu succes
    if (strstr(reply, "HTTP/1.1 201 CREATED") || strstr(reply, "HTTP/1.1 200 OK")) {
        char *collection_body = json_body(reply);
        JSON_Value *collection_json = json_parse_string(collection_body);
        JSON_Object* collection_object = json_value_get_object(collection_json);
        collection_id = (long)json_object_get_number(collection_object, "id");
    } else {
        server_error(json_body(reply)); // Altfel, afisam eroarea primita
    }
    
    // Astfel, pentru fiecare film ce dorim sa-l adaugam, trimitem o cerere cu datele acestuia
    // catre server
    if (operations_successful && num_movies > 0 && collection_id >= 0) {
        for (int i = 0; i < num_movies; ++i) {
            // Cream obiectul JSON cu datele unui singur film
            JSON_Value *movie_root = json_value_init_object();
            JSON_Object *movie = json_value_get_object(movie_root);
            json_object_set_number(movie, "id", movie_ids[i]);
            char *movie_data = json_serialize_to_string(movie_root);
            char movie_path[1000];
            snprintf(movie_path, sizeof(movie_path), "/api/v1/tema/library/collections/%ld/movies", collection_id);

            // Trimitem cererea de adaugare si asteptam raspuns
            sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
            request = compute_post_request(IP, movie_path, "application/json",
                                             movie_data, user_cookie, jwt_token);
            send_to_server(sockfd, request);
            reply = receive_from_server(sockfd);
            close(sockfd);

            // Daca nu am primit un raspuns pozitiv pentru adaugarea unui film, inseamna ca nu am reusit sa adaugam colectia
            if (!(reply != NULL && (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 201 CREATED")))) {
                operations_successful = 0; 
                break; 
            }
        }
    }

    if (operations_successful) {
        printf("SUCCESS: Colectie adaugata\n");
    } else {
        printf("ERROR: Operatiunea de adaugare a filmelor nu a fost terminata\n"); 
    }
}

void get_collections() {
    char *request = NULL;
    char *reply = NULL;
    int sockfd;

    // Verificam daca clientul are token-ul JWT
    if (jwt_token == NULL) {
        printf("ERROR: Fara acces library\n");
        return;
    }

    // Trimitem cererea catre server si asteptam raspuns
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    request = compute_get_request(IP, "/api/v1/tema/library/collections", NULL,
                                    user_cookie, jwt_token);
    send_to_server(sockfd, request);
    reply = receive_from_server(sockfd);
    close(sockfd);

    // Daca am primit un raspuns pozitiv, afisam detaliile colectiilor primite
    if (strstr(reply, "HTTP/1.1 200 OK")) {
        printf("SUCCESS: Lista colectiilor\n");
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
        server_error(json_body(reply)); // Altfel, afisam eroarea primita
    }

}

void get_collection() {
    char id[50];
    char path[1000];
    int sockfd = -1;

    // Verificam daca clientul are token-ul JWT
    if (jwt_token == NULL) {
        printf("ERROR: Fara acces library\n");
        return;
    }

    // Citim input-ul si stabilim calea corespunzatoare
    read_input("id", id, sizeof(id));
    snprintf(path, sizeof(path), "/api/v1/tema/library/collections/%s", id);

    // Trimitem cererea catre server si asteptam raspuns
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    char* request = compute_get_request(IP, path, NULL, user_cookie, jwt_token);
    send_to_server(sockfd, request);
    char* reply = receive_from_server(sockfd);
    close(sockfd);

    // Daca am primit un raspuns pozitiv, afisam datele colectiei
    if (strstr(reply, "HTTP/1.1 200 OK")) {
        JSON_Value *root = json_parse_string(json_body(reply));
        JSON_Object *collection = json_value_get_object(root);
        printf("SUCCESS: Detalii colectie\n");
        const char *title = json_object_get_string(collection, "title");
        printf("title: %s\n", title);
        const char *owner = json_object_get_string(collection, "owner");
        printf("owner: %s\n", owner);
        JSON_Array *movies = json_object_get_array(collection, "movies");
        size_t movies_count = json_array_get_count(movies);

        // Afisam datele fiecarui film din colectie
        for (size_t i = 0; i < movies_count; i++) {
            JSON_Object *movie_data = json_array_get_object(movies, i);
            long movie_id = (long)json_object_get_number(movie_data, "id");
            const char *movie_title = json_object_get_string(movie_data, "title");
            printf("#%ld: %s\n", movie_id, movie_title);
        }
    } else {
        server_error(json_body(reply)); // Altfel, afisam eroarea
    }

}

void delete_collection() {
    char id[50];
    char path[1000];
    int sockfd;

    // Verificam daca clientul are token-ul JWT
    if (jwt_token == NULL) {
        printf("ERROR: Fara acces library\n");
        return;
    }

    // Citim input-ul si stabilim calea
    read_input("id", id, sizeof(id));
    snprintf(path, sizeof(path), "/api/v1/tema/library/collections/%s", id);

    // Trimitem catre server cererea si asteptam raspuns
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    char* request = compute_delete_request(IP, path, user_cookie, jwt_token);
    send_to_server(sockfd, request);
    char* reply = receive_from_server(sockfd);
    close(sockfd);

    // Daca am primit un raspuns pozitiv, afisam un mesaj de succes,
    // altfel afisam eroarea primita
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 204 No Content")) {
        printf("SUCCESS: Colectie stearsa\n");
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

    // Verificam daca clientul are token-ul JWT
    if (jwt_token == NULL) {
        printf("ERROR: Fara acces library\n");
        return;
    }

    // Citim input-ul primit de la tastatura
    read_input("collection_id", collection_id, sizeof(collection_id));
    read_input("movie_id", movie_id, sizeof(movie_id));

    // Verificam validitatea datelor
    if (collection_id[0] == '\0' || movie_id[0] == '\0') {
        printf("ERROR: Date invalide/incomplete.\n");
        return;
    }
    
    // Stocam calea corespunzatoare din server
    snprintf(path, sizeof(path), "/api/v1/tema/library/collections/%s/movies", collection_id);

    // Initializam un obiect JSON pentru transmiterea datelor
    root = json_value_init_object();
    object = json_value_get_object(root);
    json_object_set_number(object, "id", atol(movie_id));
    data = json_serialize_to_string(root);

    // Trimitem cererea catre server si asteptam raspuns
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    char* request = compute_post_request(IP, path, "application/json", data, user_cookie, jwt_token);
    send_to_server(sockfd, request);
    char* reply = receive_from_server(sockfd);
    close(sockfd);

    // Daca am primit un raspuns pozitiv, afisam un mesaj de succes,
    // Altfel afisam eroarea primita
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 201 CREATED") || strstr(reply, "HTTP/1.1 204 NO CONTENT")) {
        printf("SUCCESS: Film adaugat in colectie\n");
    } else {
        server_error(json_body(reply));
    }
}

void delete_movie_from_collection() {
    char collection_id[50];
    char movie_id[50];
    char path[1000];

    int sockfd = -1;

    // Verificam daca clientul are token-ul JWT
    if (jwt_token == NULL) {
        printf("ERROR: Fara acces library\n");
        return;
    }

    // Citim input-ul primit de la tastatura si stabilim calea pentru server
    read_input("collection_id", collection_id, sizeof(collection_id));
    read_input("movie_id", movie_id, sizeof(movie_id));
    snprintf(path, sizeof(path), "/api/v1/tema/library/collections/%s/movies/%s", collection_id, movie_id);

    // Trimitem cererea catre server si asteptam raspuns
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    char* request = compute_delete_request(IP, path, user_cookie, jwt_token);
    send_to_server(sockfd, request);
    char* reply = receive_from_server(sockfd);
    close(sockfd);
    
    // Daca primim un raspuns pozitiv, afisam un mesaj de succes,
    // altfel afisam eroarea primita
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 204 No Content")) {
        printf("SUCCESS: Film sters din colectie\n");
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

    // Verificam daca clientul este
    // autentificat ca admin
    if (admin_cookie == NULL) {
        printf("ERROR: Lipsa rol admin\n");
        return;
    }

    // Citim inputul si stabilim calea pentru server
    read_input("username", user, sizeof(user));
    snprintf(path, sizeof(path), "/api/v1/tema/admin/users/%s", user);

    // Verificam validitatea datelor
    if (user[0] == '\0') {
        printf("ERROR: Numele de utilizator nu poate fi gol.\n");
        return;
    }

    // Transmitem cererea catre server si asteptam raspuns
    sockfd = open_connection(IP, PORT, AF_INET, SOCK_STREAM, 0);
    request = compute_delete_request(IP, path, admin_cookie, NULL); 
    send_to_server(sockfd, request);
    reply = receive_from_server(sockfd);
    close(sockfd);

    // Daca am primit un raspuns pozitiv, afisam un mesaj de succes,
    // altfel, afisam eroarea primita
    if (strstr(reply, "HTTP/1.1 200 OK") || strstr(reply, "HTTP/1.1 204 No Content")) {
        printf("SUCCESS: Utilizator sters\n");
    } else {
        server_error(json_body(reply));
    }
}

int main() {
    char command[50];

    while(1) {
        // Citim cate o comanda pe rand
        fgets(command, sizeof(command), stdin);
        command[strlen(command) - 1] = '\0';

        // Verificam ce tip de comanda am primit, si apelam
        // functia corespunzatoare
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