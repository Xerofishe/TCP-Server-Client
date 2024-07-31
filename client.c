#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#define PORT 3672
#define BUFFER_SIZE 100

void DieWithError(const char *errmsg) {
    fprintf(stderr, "%s: %s\n", errmsg, strerror(errno));
    exit(EXIT_FAILURE);
}

void send_file(int sockfd, const char *filename) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    FILE *file = fopen(filename, "r");  // Open in text mode
    if (!file) {
		DieWithError("Error opening file");
    }

    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE - 1, file)) > 0) {
        buffer[bytes_read] = '\0';  // Null-terminate the string
        if (send(sockfd, buffer, bytes_read, 0) < 0) {
            fclose(file);
            DieWithError("Send failed");
        }
    }
    fclose(file);
}


void receive_file(int sockfd, const char *filename) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    FILE *file = fopen(filename, "w");
    if (!file) {
        DieWithError("Cant open file.");
    }

    int bytes_received;
    while ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0)) > 0) {
        fwrite(buffer, 1, bytes_received-1, file);
        if (bytes_received < BUFFER_SIZE) break;
    }
    fclose(file);
}

int main() {
    struct sockaddr_in tcpServer;
    char buffer[BUFFER_SIZE];
    int sockfd, recv_size;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        DieWithError("Socket creation failed");

    memset(&tcpServer, 0, sizeof(tcpServer));
    tcpServer.sin_family = AF_INET;
    tcpServer.sin_port = htons(PORT);
    tcpServer.sin_addr.s_addr = inet_addr("127.0.0.1");

    if((connect(sockfd, (struct sockaddr*)&tcpServer, sizeof(tcpServer))) < 0)
        DieWithError("Connection failed");

    // Authentication process
    for (int i = 0; i < 2; i++) {
        if ((recv_size = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) < 0)
            DieWithError("Receive failed");
        buffer[recv_size] = '\0';
        printf("%s", buffer);

        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';
        if (send(sockfd, buffer, strlen(buffer), 0) < 0)
            DieWithError("Send failed");
    }

    // Receive authentication result
    if ((recv_size = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) < 0)
        DieWithError("Receive failed");
    buffer[recv_size] = '\0';
    printf("%s", buffer);

    if (strncmp(buffer, "Authentication Successful", 25) != 0) {
        close(sockfd);
        return 1;
    }

    // Main communication loop
    while(1) {
    char command[20], filename[100];
	memset(command, 0, sizeof(command));
	memset(filename, 0, sizeof(filename));
    printf("Enter command (UPLOAD, DOWNLOAD, ECHO, CLOSE): ");
    fgets(command, sizeof(command), stdin);
    command[strcspn(command, "\n")] = '\0';

    if (send(sockfd, command, strlen(command), 0) < 0)
        DieWithError("Send failed");

    if (strcmp(command, "UPLOAD") == 0) {
        printf("Enter filename to upload: ");
        fgets(filename, sizeof(filename), stdin);
        filename[strcspn(filename, "\n")] = '\0';
        if (send(sockfd, filename, strlen(filename), 0) < 0)
            DieWithError("Send failed");
        
        char response[10];
		memset(response, 0, sizeof(response));
        recv(sockfd, response, sizeof(response), 0);
        if (strcmp(response, "OK") == 0) {
			printf("%s", response);
            send_file(sockfd, filename);
            printf("File uploaded successfully.\n");
        } else {
            printf("Error uploading file.\n");
        }
    } else if (strcmp(command, "DOWNLOAD") == 0) {
    printf("Enter filename to download: ");
    fgets(filename, sizeof(filename), stdin);
	filename[strcspn(filename, "\n")] = '\0';
	if (send(sockfd, filename, strlen(filename), 0) < 0)
        DieWithError("Send failed");
  
        receive_file(sockfd, filename);
	} else if (strcmp(command, "ECHO") == 0) {
		printf("Enter message: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';
        if (send(sockfd, buffer, sizeof(buffer), 0) < 0)
            DieWithError("Send failed");
        if ((recv_size = recv(sockfd, buffer, BUFFER_SIZE, 0)) < 0)
            DieWithError("Receive failed");
        printf("Server response: %s\n", buffer);
    } else if (strcmp(command, "CLOSE") == 0) {
        break;
    } else {
        printf("Invalid command.\n");
		memset(command, 0, sizeof(command));
    }
}

    close(sockfd);
    return 0;
}
