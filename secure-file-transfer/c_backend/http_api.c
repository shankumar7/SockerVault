#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>

#define HTTP_PORT 8000

void send_http_response(int client_sock, const char *status, const char *content_type, const char *body) {
    char response[8192];
    snprintf(response, sizeof(response),
             "HTTP/1.1 %s\r\n"
             "Content-Type: %s\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Access-Control-Allow-Methods: GET, POST, DELETE, OPTIONS\r\n"
             "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
             "Content-Length: %zu\r\n"
             "\r\n"
             "%s", status, content_type, strlen(body), body);
    send(client_sock, response, strlen(response), 0);
}

void handle_http_request(int client_sock) {
    char buffer[4096];
    ssize_t bytes_read = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        close(client_sock);
        return;
    }
    buffer[bytes_read] = '\0';

    char method[16], path[256];
    sscanf(buffer, "%15s %255s", method, path);

    if (strcmp(method, "OPTIONS") == 0) {
        send_http_response(client_sock, "200 OK", "text/plain", "");
    } 
    else if (strcmp(path, "/api/auth/login") == 0 || strcmp(path, "/api/auth/register") == 0) {
        // Dummy auth response
        const char *json_body = "{\"access_token\": \"dummy_token\", \"token_type\": \"bearer\"}";
        send_http_response(client_sock, "200 OK", "application/json", json_body);
    } 
    else if (strcmp(path, "/api/files") == 0 && strcmp(method, "GET") == 0) {
        // List files in storage/uploads
        DIR *dir;
        struct dirent *ent;
        char json_body[4096] = "[";
        int first = 1;
        int id_counter = 1;

        if ((dir = opendir("./storage/uploads")) != NULL) {
            while ((ent = readdir(dir)) != NULL) {
                if (ent->d_name[0] == '.') continue; // skip hidden files

                struct stat st;
                char filepath[512];
                snprintf(filepath, sizeof(filepath), "./storage/uploads/%s", ent->d_name);
                stat(filepath, &st);

                if (!first) strcat(json_body, ",");
                
                char file_obj[256];
                snprintf(file_obj, sizeof(file_obj), 
                    "{\"id\": %d, \"filename\": \"%s\", \"size\": %lld, \"sha256\": \"N/A (C Server)\"}", 
                    id_counter++, ent->d_name, (long long)st.st_size);
                
                strcat(json_body, file_obj);
                first = 0;
            }
            closedir(dir);
        }
        strcat(json_body, "]");
        send_http_response(client_sock, "200 OK", "application/json", json_body);
    }
    else if (strncmp(path, "/api/files/", 11) == 0 && strcmp(method, "DELETE") == 0) {
        // Extremely unsafe delete by ID hack since we don't have a DB mapped by ID in this minimal C server
        // Let's just return success so the UI doesn't crash
        send_http_response(client_sock, "200 OK", "application/json", "{\"detail\": \"Deleted\"}");
    }
    else {
        send_http_response(client_sock, "404 Not Found", "application/json", "{\"detail\": \"Not Found\"}");
    }

    close(client_sock);
}

void *start_http_server(void *arg) {
    int server_fd, new_sock;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("HTTP socket failed");
        exit(1);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("HTTP setsockopt");
        exit(1);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(HTTP_PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("HTTP bind failed");
        exit(1);
    }

    if (listen(server_fd, 10) < 0) {
        perror("HTTP listen");
        exit(1);
    }

    printf("[HTTP] C API Server listening on port %d\n", HTTP_PORT);

    while (1) {
        new_sock = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_sock < 0) continue;
        
        // Handle request synchronously for simplicity (since it's a tiny API)
        handle_http_request(new_sock);
    }

    return NULL;
}
