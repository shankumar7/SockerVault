#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 9000
#define CHUNK_SIZE 65536

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr,"Usage: %s <server_ip> <upload/download> <filename>\n", argv[0]);
        exit(1);
    }

    char *server_ip = argv[1];
    char *command = argv[2];
    char *filename = argv[3];

    int sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if(inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0)
        error("ERROR invalid server IP");

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
        
    printf("Connected to TCP Server at %s:%d\n", server_ip, PORT);

    // Simple protocol for C:
    // Send 1 byte command (U for upload, D for download)
    // Send 255 byte filename
    char cmd_char = (command[0] == 'u' || command[0] == 'U') ? 'U' : 'D';
    send(sockfd, &cmd_char, 1, 0);
    
    char name_buffer[255] = {0};
    strncpy(name_buffer, filename, 254);
    send(sockfd, name_buffer, 255, 0);

    if (cmd_char == 'U') {
        FILE *fp = fopen(filename, "rb");
        if (!fp) error("ERROR opening file for upload");
        
        char buffer[CHUNK_SIZE];
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, fp)) > 0) {
            send(sockfd, buffer, bytes_read, 0);
        }
        fclose(fp);
        printf("Upload complete.\n");
    } 
    else if (cmd_char == 'D') {
        FILE *fp = fopen(filename, "wb");
        if (!fp) error("ERROR creating file for download");
        
        char buffer[CHUNK_SIZE];
        ssize_t bytes_received;
        while ((bytes_received = recv(sockfd, buffer, CHUNK_SIZE, 0)) > 0) {
            fwrite(buffer, 1, bytes_received, fp);
        }
        fclose(fp);
        printf("Download complete.\n");
    }

    close(sockfd);
    return 0;
}
