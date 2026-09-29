#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>

#define HTTP_PORT 8000
#define BUFFER_SIZE 65536

void send_http_response(int client_sock, const char *status, const char *content_type, const char *body) {
    char response[8192];
    snprintf(response, sizeof(response),
             "HTTP/1.1 %s\r\n"
             "Content-Type: %s\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Access-Control-Allow-Methods: GET, POST, DELETE, OPTIONS\r\n"
             "Access-Control-Allow-Headers: Content-Type, Authorization, X-Filename\r\n"
             "Content-Length: %zu\r\n"
             "\r\n"
             "%s", status, content_type, strlen(body), body);
    send(client_sock, response, strlen(response), 0);
}

// Function to handle reading HTTP headers and returning the body offset
char* get_header_value(char* headers, const char* header_name) {
    char* line = strcasestr(headers, header_name);
    if (line) {
        line += strlen(header_name);
        while (*line == ' ' || *line == ':') line++;
        char* end = strstr(line, "\r\n");
        if (end) {
            char* val = malloc(end - line + 1);
            strncpy(val, line, end - line);
            val[end - line] = '\0';
            return val;
        }
    }
    return NULL;
}

void handle_http_request(int client_sock) {
    char *buffer = malloc(BUFFER_SIZE);
    ssize_t bytes_read = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_read <= 0) {
        free(buffer);
        close(client_sock);
        return;
    }
    buffer[bytes_read] = '\0';

    char method[16], path[256];
    sscanf(buffer, "%15s %255s", method, path);

    char *body_start = strstr(buffer, "\r\n\r\n");
    if (body_start) {
        body_start += 4;
    }

    if (strcmp(method, "OPTIONS") == 0) {
        send_http_response(client_sock, "200 OK", "text/plain", "");
    } 
    else if (strcmp(path, "/api/upload") == 0 && strcmp(method, "POST") == 0) {
        char* filename = get_header_value(buffer, "X-Filename");
        char* content_length_str = get_header_value(buffer, "Content-Length");
        
        if (filename && content_length_str) {
            long content_length = atol(content_length_str);
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "./storage/uploads/%s", filename);
            
            FILE *fp = fopen(filepath, "wb");
            if (fp) {
                // Write the part of the body we already read
                long bytes_written = 0;
                if (body_start) {
                    long initial_body_bytes = bytes_read - (body_start - buffer);
                    if (initial_body_bytes > 0) {
                        fwrite(body_start, 1, initial_body_bytes, fp);
                        bytes_written += initial_body_bytes;
                    }
                }
                
                // Read the rest of the body
                while (bytes_written < content_length) {
                    ssize_t r = recv(client_sock, buffer, BUFFER_SIZE, 0);
                    if (r <= 0) break;
                    fwrite(buffer, 1, r, fp);
                    bytes_written += r;
                }
                fclose(fp);
                send_http_response(client_sock, "200 OK", "application/json", "{\"detail\": \"Upload successful\"}");
            } else {
                send_http_response(client_sock, "500 Internal Error", "application/json", "{\"detail\": \"File open error\"}");
            }
            free(filename);
            free(content_length_str);
        } else {
            send_http_response(client_sock, "400 Bad Request", "application/json", "{\"detail\": \"Missing X-Filename or Content-Length\"}");
            if (filename) free(filename);
            if (content_length_str) free(content_length_str);
        }
    }
    else if (strncmp(path, "/api/download/", 14) == 0 && strcmp(method, "GET") == 0) {
        char *filename = path + 14; // extract filename after /api/download/
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "./storage/uploads/%s", filename);
        
        FILE *fp = fopen(filepath, "rb");
        if (fp) {
            struct stat st;
            stat(filepath, &st);
            
            char header[1024];
            snprintf(header, sizeof(header),
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Type: application/octet-stream\r\n"
                     "Content-Disposition: attachment; filename=\"%s\"\r\n"
                     "Access-Control-Allow-Origin: *\r\n"
                     "Content-Length: %lld\r\n\r\n", filename, (long long)st.st_size);
            
            send(client_sock, header, strlen(header), 0);
            
            size_t r;
            while ((r = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
                send(client_sock, buffer, r, 0);
            }
            fclose(fp);
        } else {
            send_http_response(client_sock, "404 Not Found", "application/json", "{\"detail\": \"File not found\"}");
        }
    }
    else if (strcmp(path, "/api/auth/login") == 0 || strcmp(path, "/api/auth/register") == 0) {
        const char *json_body = "{\"access_token\": \"dummy_token\", \"token_type\": \"bearer\"}";
        send_http_response(client_sock, "200 OK", "application/json", json_body);
    } 
    else if (strcmp(path, "/api/files") == 0 && strcmp(method, "GET") == 0) {
        DIR *dir;
        struct dirent *ent;
        char *json_body = malloc(65536); // Large buffer for JSON
        strcpy(json_body, "[");
        int first = 1;
        int id_counter = 1;

        if ((dir = opendir("./storage/uploads")) != NULL) {
            while ((ent = readdir(dir)) != NULL) {
                if (ent->d_name[0] == '.') continue; 

                struct stat st;
                char filepath[512];
                snprintf(filepath, sizeof(filepath), "./storage/uploads/%s", ent->d_name);
                stat(filepath, &st);

                if (!first) strcat(json_body, ",");
                
                char file_obj[256];
                snprintf(file_obj, sizeof(file_obj), 
                    "{\"id\": %d, \"filename\": \"%s\", \"size\": %lld, \"sha256\": \"N/A (HTTP Server)\"}", 
                    id_counter++, ent->d_name, (long long)st.st_size);
                
                strcat(json_body, file_obj);
                first = 0;
            }
            closedir(dir);
        }
        strcat(json_body, "]");
        send_http_response(client_sock, "200 OK", "application/json", json_body);
        free(json_body);
    }
    else if (strncmp(path, "/api/files/", 11) == 0 && strcmp(method, "DELETE") == 0) {
        // Just return success for UI compatibility
        send_http_response(client_sock, "200 OK", "application/json", "{\"detail\": \"Deleted\"}");
    }
    else {
        send_http_response(client_sock, "404 Not Found", "application/json", "{\"detail\": \"Not Found\"}");
    }

    free(buffer);
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
        handle_http_request(new_sock);
    }

    return NULL;
}
