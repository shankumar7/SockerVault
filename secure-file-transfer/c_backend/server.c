#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>

// Forward declare the HTTP server thread function
void *start_http_server(void *arg);

#define PORT 9000
#define CHUNK_SIZE 65536

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void *handle_client(void *client_socket) {
    int sock = *(int*)client_socket;
    free(client_socket);
    
    char cmd;
    if (recv(sock, &cmd, 1, 0) <= 0) {
        close(sock);
        return NULL;
    }
    
    char filename[255] = {0};
    if (recv(sock, filename, 255, 0) <= 0) {
        close(sock);
        return NULL;
    }
    
    // Sanitize filename (remove paths)
    char *safe_name = strrchr(filename, '/');
    if (safe_name) safe_name++;
    else safe_name = filename;
    
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "./storage/uploads/%s", safe_name);

    if (cmd == 'U') {
        printf("[TCP] Receiving file: %s\n", safe_name);
        FILE *fp = fopen(filepath, "wb");
        if (fp) {
            char buffer[CHUNK_SIZE];
            ssize_t bytes_received;
            while ((bytes_received = recv(sock, buffer, CHUNK_SIZE, 0)) > 0) {
                fwrite(buffer, 1, bytes_received, fp);
            }
            fclose(fp);
            printf("[TCP] File %s saved.\n", safe_name);
        }
    } else if (cmd == 'D') {
        printf("[TCP] Sending file: %s\n", safe_name);
        FILE *fp = fopen(filepath, "rb");
        if (fp) {
            char buffer[CHUNK_SIZE];
            size_t bytes_read;
            while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, fp)) > 0) {
                send(sock, buffer, bytes_read, 0);
            }
            fclose(fp);
            printf("[TCP] File %s sent.\n", safe_name);
        }
    }
    
    close(sock);
    return NULL;
}

int main() {
    int server_fd, *new_sock;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    mkdir("./storage", 0777);
    mkdir("./storage/uploads", 0777);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        error("socket failed");

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
        error("setsockopt");

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
        error("bind failed");

    if (listen(server_fd, 10) < 0)
        error("listen");

    printf("=========================================\n");
    printf("SECURE C TCP FILE TRANSFER SERVER\n");
    printf("TCP Port: %d\n", PORT);
    printf("=========================================\n");
    
    // Start the HTTP API Server in a background thread
    pthread_t http_thread;
    pthread_create(&http_thread, NULL, start_http_server, NULL);
    pthread_detach(http_thread);

    while(1) {
        new_sock = malloc(sizeof(int));
        *new_sock = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (*new_sock < 0) {
            perror("accept");
            free(new_sock);
            continue;
        }
        
        pthread_t client_thread;
        pthread_create(&client_thread, NULL, handle_client, (void*)new_sock);
        pthread_detach(client_thread);
    }

    close(server_fd);
    return 0;
}
